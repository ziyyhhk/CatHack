#include "Hacks.hpp"

#include <Geode/Geode.hpp>
#include <imgui-cocos.hpp>
#include <imgui.h>
#include <imgui_internal.h>

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <string>
#include <unordered_map>

using namespace geode::prelude;

namespace {

// ---------------------------------------------------------------- look
constexpr ImU32 kPink  = IM_COL32(232, 56, 104, 255);
constexpr ImU32 kWhite = IM_COL32(255, 255, 255, 255);
constexpr ImU32 kText  = IM_COL32(226, 226, 226, 255);
constexpr ImU32 kDim   = IM_COL32(105, 105, 105, 255);
constexpr ImU32 kTrack = IM_COL32(22, 22, 22, 255);
constexpr ImU32 kBlack = IM_COL32(0, 0, 0, 255);

// animation lengths in seconds
constexpr float kRowAnim = 0.18f;
constexpr float kCollapseAnim = 0.20f;
constexpr float kFadeAnim = 0.14f;
constexpr int kMenuLayoutVersion = 3;

// ---------------------------------------------------------------- state
struct WinState {
    ImVec2 pos{0.f, 0.f};
    bool hasPos = false;
    bool collapsed = false;
    float openT = 1.f; // 1 = open, 0 = collapsed
};

bool g_open = false;
float g_fade = 0.f;
float g_scale = 0.8f; // applied scale (only updated after you let go of the slider)
float g_savedViewportWidth = 0.f;
float g_savedViewportHeight = 0.f;
float g_savedLayoutScale = 0.f;
int g_savedColumnsPerRow = 0;
bool g_resetWindowPositions = true;
std::unordered_map<std::string, WinState> g_win;

// ---------------------------------------------------------------- small helpers
float ease(float t) {
    t = std::clamp(t, 0.f, 1.f);
    return t * t * (3.f - 2.f * t);
}

float approach(float cur, float target, float dt, float duration) {
    float step = dt / duration;
    return cur < target ? std::min(cur + step, target) : std::max(cur - step, target);
}

// Colors passed straight to a draw list ignore the style alpha, so multiply it
// in by hand. This is what makes the whole menu fade.
ImU32 withAlpha(ImU32 c, float a = 1.f) {
    a = std::clamp(a * ImGui::GetStyle().Alpha, 0.f, 1.f);
    auto alpha = static_cast<ImU32>(static_cast<float>((c >> IM_COL32_A_SHIFT) & 0xFF) * a);
    return (c & ~static_cast<ImU32>(IM_COL32_A_MASK)) | (alpha << IM_COL32_A_SHIFT);
}

ImU32 mix(ImU32 a, ImU32 b, float t) {
    auto channel = [&](int shift) {
        float x = static_cast<float>((a >> shift) & 0xFF);
        float y = static_cast<float>((b >> shift) & 0xFF);
        return static_cast<ImU32>(x + (y - x) * t);
    };
    return (channel(IM_COL32_R_SHIFT) << IM_COL32_R_SHIFT) |
           (channel(IM_COL32_G_SHIFT) << IM_COL32_G_SHIFT) |
           (channel(IM_COL32_B_SHIFT) << IM_COL32_B_SHIFT) |
           (0xFFu << IM_COL32_A_SHIFT);
}

// Smoothly animated 0..1 value kept in the current window's state storage.
// The first time it is seen it starts at the target, so nothing pops in.
float animated(ImGuiID id, float target, float dt, float duration) {
    ImGuiStorage* st = ImGui::GetStateStorage();
    float v = approach(st->GetFloat(id, target), target, dt, duration);
    st->SetFloat(id, v);
    return ease(v);
}

float rowHeight() {
    return std::floor(ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.f + 0.5f);
}

std::string fitText(const char* text, float maxWidth, bool& wasTruncated) {
    std::string fitted = text ? text : "";
    if (ImGui::CalcTextSize(fitted.c_str()).x <= maxWidth) return fitted;

    wasTruncated = true;
    while (!fitted.empty()) {
        fitted.pop_back();
        std::string candidate = fitted + "...";
        if (ImGui::CalcTextSize(candidate.c_str()).x <= maxWidth) return candidate;
    }
    return "";
}

float windowExpandedHeight(cat::Window const& w, float rowH) {
    int bodyRows = static_cast<int>(w.entries.size()) + (w.extra != cat::Extra::None ? 1 : 0);
    return rowH * static_cast<float>(bodyRows + 1) + 2.f;
}

float windowHeight(cat::Window const& w, WinState const& ws, float rowH, float maxHeight) {
    maxHeight = std::max(maxHeight, rowH + 2.f);
    float openHeight = std::max(rowH + 2.f, std::min(windowExpandedHeight(w, rowH), maxHeight));
    return rowH + (openHeight - rowH) * ws.openT;
}

// ---------------------------------------------------------------- config
void saveConfig() {
    auto* mod = Mod::get();
    auto& s = cat::state();

    for (auto& w : cat::layout()) {
        for (auto& e : w.entries) {
            if (e.ptr && e.id) mod->setSavedValue<bool>(e.id, *e.ptr);
        }
    }
    mod->setSavedValue<double>("speed", s.speed);
    mod->setSavedValue<double>("fps", s.fps);
    mod->setSavedValue<double>("interface-scale", s.interfaceScale);
    mod->setSavedValue<int>("menu-layout-version", kMenuLayoutVersion);
    mod->setSavedValue<double>("menu-viewport-width", g_savedViewportWidth);
    mod->setSavedValue<double>("menu-viewport-height", g_savedViewportHeight);
    mod->setSavedValue<double>("menu-layout-scale", g_savedLayoutScale);
    mod->setSavedValue<int>("menu-grid-columns", g_savedColumnsPerRow);

    for (auto& [title, ws] : g_win) {
        mod->setSavedValue<bool>("collapsed-" + title, ws.collapsed);
        auto xKey = "pos-" + title + "-x";
        auto yKey = "pos-" + title + "-y";
        if (ws.hasPos) {
            mod->setSavedValue<double>(xKey, ws.pos.x);
            mod->setSavedValue<double>(yKey, ws.pos.y);
        } else {
            mod->setSavedValue<double>(xKey, -1.0);
            mod->setSavedValue<double>(yKey, -1.0);
        }
    }
}

void loadConfig() {
    auto* mod = Mod::get();
    auto& s = cat::state();

    int savedLayoutVersion = mod->getSavedValue<int>("menu-layout-version", 0);
    g_resetWindowPositions = savedLayoutVersion != kMenuLayoutVersion;
    g_savedViewportWidth = static_cast<float>(mod->getSavedValue<double>("menu-viewport-width", 0.0));
    g_savedViewportHeight = static_cast<float>(mod->getSavedValue<double>("menu-viewport-height", 0.0));
    g_savedLayoutScale = static_cast<float>(mod->getSavedValue<double>("menu-layout-scale", 0.0));
    g_savedColumnsPerRow = mod->getSavedValue<int>("menu-grid-columns", 0);

    for (auto& w : cat::layout()) {
        for (auto& e : w.entries) {
            if (e.ptr && e.id) *e.ptr = mod->getSavedValue<bool>(e.id, *e.ptr);
        }

        std::string title = w.title;
        auto& ws = g_win[title];
        ws.collapsed = mod->getSavedValue<bool>("collapsed-" + title, false);
        ws.openT = ws.collapsed ? 0.f : 1.f;
        if (!g_resetWindowPositions) {
            double x = mod->getSavedValue<double>("pos-" + title + "-x", -1.0);
            double y = mod->getSavedValue<double>("pos-" + title + "-y", -1.0);
            if (x >= 0.0 && y >= 0.0) {
                ws.pos = ImVec2(static_cast<float>(x), static_cast<float>(y));
                ws.hasPos = true;
            }
        }
    }

    s.speed = static_cast<float>(mod->getSavedValue<double>("speed", s.speed));
    s.fps = static_cast<float>(mod->getSavedValue<double>("fps", s.fps));
    if (savedLayoutVersion < kMenuLayoutVersion) {
        // The previous default was oversized; start the compact layout at a
        // smaller scale once, then preserve the user's future slider choice.
        s.interfaceScale = 0.8f;
    } else {
        s.interfaceScale = std::clamp(
            static_cast<float>(mod->getSavedValue<double>("interface-scale", s.interfaceScale)),
            0.65f,
            1.5f
        );
    }
    g_scale = s.interfaceScale;
}

void applyFps() {
    auto& s = cat::state();
    if (s.fpsEnabled && s.fps > 0.f) {
        CCApplication::sharedApplication()->setAnimationInterval(1.0 / static_cast<double>(s.fps));
    } else {
        CCApplication::sharedApplication()->setAnimationInterval(1.0 / 60.0);
    }
}

void setOpen(bool open) {
    g_open = open;
    if (!open) saveConfig();
}

void applyTheme() {
    ImGuiIO& io = ImGui::GetIO();
    auto fontPath = Mod::get()->getResourcesDir() / "fonts" / "DejaVuSans.ttf";
    if (std::filesystem::exists(fontPath)) {
        ImFont* font = io.Fonts->AddFontFromFileTTF(fontPath.string().c_str(), 14.f);
        if (font) {
            io.FontDefault = font;
        } else {
            log::warn("Could not load the bundled DejaVu Sans font; keeping ImGui's fallback font.");
        }
    } else {
        log::warn("Bundled DejaVu Sans font is missing; keeping ImGui's fallback font.");
    }

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 3.f;
    style.WindowBorderSize = 0.f;
    style.WindowPadding = ImVec2(0.f, 0.f);
    style.FramePadding = ImVec2(4.f, 1.f);
    style.ItemSpacing = ImVec2(0.f, 0.f);
    style.ScrollbarSize = 6.f;

    ImVec4* c = style.Colors;
    c[ImGuiCol_WindowBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
    c[ImGuiCol_Border] = ImVec4(0.f, 0.f, 0.f, 0.f);
    c[ImGuiCol_Text] = ImVec4(0.89f, 0.89f, 0.89f, 1.f);
    c[ImGuiCol_FrameBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.f);
    c[ImGuiCol_FrameBgHovered] = ImVec4(0.16f, 0.16f, 0.16f, 1.f);
    c[ImGuiCol_FrameBgActive] = ImVec4(0.18f, 0.18f, 0.18f, 1.f);
    c[ImGuiCol_SliderGrab] = ImVec4(0.91f, 0.22f, 0.41f, 1.f);
    c[ImGuiCol_SliderGrabActive] = ImVec4(1.f, 0.35f, 0.55f, 1.f);
    c[ImGuiCol_CheckMark] = ImVec4(0.91f, 0.22f, 0.41f, 1.f);
}

// ---------------------------------------------------------------- drawing
void drawShadow(ImVec2 pos, ImVec2 size) {
    auto* dl = ImGui::GetBackgroundDrawList();
    ImU32 shadow = withAlpha(IM_COL32(0, 0, 0, 80));
    dl->AddRectFilled(ImVec2(pos.x + 3.f, pos.y + 4.f), ImVec2(pos.x + size.x + 3.f, pos.y + size.y + 4.f), shadow, 4.f);
}

void titleBar(cat::Window const& w, WinState& ws, int enabled) {
    ImVec2 p = ImGui::GetCursorScreenPos();
    float wW = ImGui::GetContentRegionAvail().x;
    float h = rowHeight();
    ImVec2 size(wW, h);

    auto* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(p, ImVec2(p.x + size.x, p.y + size.y), withAlpha(kPink), 0.f);

    // +/- button
    float btn = h * 0.55f;
    ImVec2 bMin(p.x + 3.f, p.y + (h - btn) * 0.5f);
    ImVec2 bMax(bMin.x + btn, bMin.y + btn);
    float t = ws.openT;
    ImU32 btnCol = withAlpha(kWhite);
    // horizontal bar always
    dl->AddRectFilled(ImVec2(bMin.x + 2.f, bMin.y + btn * 0.45f), ImVec2(bMax.x - 2.f, bMin.y + btn * 0.55f), btnCol);
    // vertical bar fades out when open
    if (t < 0.999f) {
        float a = 1.f - t;
        ImU32 vert = withAlpha(IM_COL32(255, 255, 255, static_cast<int>(255 * a)));
        dl->AddRectFilled(ImVec2(bMin.x + btn * 0.45f, bMin.y + 2.f), ImVec2(bMin.x + btn * 0.55f, bMax.y - 2.f), vert);
    }

    char titleBuf[128];
    if (ws.collapsed && enabled > 0) {
        std::snprintf(titleBuf, sizeof(titleBuf), "%s (%d)", w.title, enabled);
    } else {
        std::snprintf(titleBuf, sizeof(titleBuf), "%s", w.title);
    }
    ImVec2 ts = ImGui::CalcTextSize(titleBuf);
    dl->AddText(ImVec2(p.x + btn + 6.f, p.y + (h - ts.y) * 0.5f), withAlpha(kWhite), titleBuf);

    ImGui::InvisibleButton(("##title-" + std::string(w.title)).c_str(), size);
    if (ImGui::IsItemClicked() || ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
        ws.collapsed = !ws.collapsed;
    }
    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0)) {
        ws.pos.x += ImGui::GetIO().MouseDelta.x;
        ws.pos.y += ImGui::GetIO().MouseDelta.y;
        ws.hasPos = true;
    }

    ws.openT = approach(ws.openT, ws.collapsed ? 0.f : 1.f, ImGui::GetIO().DeltaTime, kCollapseAnim);
}

bool toggleRow(cat::Entry const& e, bool centered) {
    ImVec2 p = ImGui::GetCursorScreenPos();
    float wW = ImGui::GetContentRegionAvail().x;
    float h = rowHeight();
    ImVec2 size(wW, h);

    bool implemented = e.ptr != nullptr;
    bool enabled = implemented && *e.ptr;
    bool hovered = false;

    ImGui::InvisibleButton((e.id ? e.id : e.label), size);
    hovered = ImGui::IsItemHovered();
    bool pressed = false;
    if (implemented && ImGui::IsItemClicked()) {
        *e.ptr = !*e.ptr;
        pressed = true;
        saveConfig();
    }

    float hoverT = animated(ImGui::GetID((std::string(e.label) + "-h").c_str()), hovered ? 1.f : 0.f, ImGui::GetIO().DeltaTime, kRowAnim);
    float enT = animated(ImGui::GetID((std::string(e.label) + "-e").c_str()), enabled ? 1.f : 0.f, ImGui::GetIO().DeltaTime, kRowAnim);

    auto* dl = ImGui::GetWindowDrawList();
    ImU32 bg = mix(withAlpha(IM_COL32(20, 20, 20, 255)), withAlpha(IM_COL32(40, 18, 28, 255)), enT);
    if (hoverT > 0.01f) bg = mix(bg, withAlpha(IM_COL32(50, 22, 35, 255)), hoverT);
    if (!implemented) bg = withAlpha(IM_COL32(16, 16, 16, 255));
    dl->AddRectFilled(p, ImVec2(p.x + size.x, p.y + size.y), bg);

    // right track bar
    float trackW = 2.f;
    ImU32 trackCol = mix(withAlpha(kTrack), withAlpha(kPink), enT);
    dl->AddRectFilled(ImVec2(p.x + size.x - trackW, p.y), ImVec2(p.x + size.x, p.y + size.y), trackCol);

    ImU32 textCol = implemented ? mix(withAlpha(kText), withAlpha(kPink), enT * 0.6f) : withAlpha(kDim);
    constexpr float textPad = 6.f;
    float textMaxWidth = std::max(1.f, size.x - trackW - textPad * 2.f);
    bool labelTruncated = false;
    std::string displayLabel = fitText(e.label, textMaxWidth, labelTruncated);
    ImVec2 ts = ImGui::CalcTextSize(displayLabel.c_str());
    float tx = centered ? p.x + (size.x - trackW - ts.x) * 0.5f : p.x + textPad;
    dl->AddText(ImVec2(tx, p.y + (h - ts.y) * 0.5f), textCol, displayLabel.c_str());

    float hoverTime = ImGui::GetCurrentContext()->HoveredIdTimer;
    bool showTooltip = hovered && (!implemented || ((e.desc || labelTruncated) && hoverTime > 0.45f));
    if (showTooltip) {
        ImGui::BeginTooltip();
        if (labelTruncated) ImGui::TextUnformatted(e.label);
        if (e.desc) ImGui::TextUnformatted(e.desc);
        else if (!implemented) ImGui::TextUnformatted("Not implemented yet");
        ImGui::EndTooltip();
    }

    return pressed;
}

void extraRow(cat::Extra extra) {
    if (extra == cat::Extra::None) return;
    auto& s = cat::state();
    ImGui::PushItemWidth(-1);
    if (extra == cat::Extra::Scale) {
        float v = s.interfaceScale;
        if (ImGui::SliderFloat("##scale", &v, 0.65f, 1.5f, "Scale %.2f")) {
            s.interfaceScale = v;
        }
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            g_scale = s.interfaceScale;
            saveConfig();
        }
    } else if (extra == cat::Extra::Speed) {
        float v = s.speed;
        if (ImGui::SliderFloat("##speed", &v, 0.1f, 3.0f, "Speed %.2fx")) {
            s.speed = v;
        }
        if (ImGui::IsItemDeactivatedAfterEdit()) saveConfig();
    } else if (extra == cat::Extra::Fps) {
        float v = s.fps;
        if (ImGui::SliderFloat("##fps", &v, 30.f, 1000.f, "FPS %.0f")) {
            s.fps = v;
        }
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            if (s.fpsEnabled) applyFps();
            saveConfig();
        }
    }
    ImGui::PopItemWidth();
}

void drawWindow(cat::Window const& w, ImVec2 defPos, float width, float rowH, float maxHeight) {
    auto& ws = g_win[w.title];
    if (!ws.hasPos) {
        ws.pos = defPos;
        ws.hasPos = true;
    }

    float height = windowHeight(w, ws, rowH, maxHeight);
    ImGui::SetNextWindowPos(ws.pos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Always);

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoSavedSettings;
    if (!g_open) flags |= ImGuiWindowFlags_NoInputs; // fading out: let clicks through

    ImGui::Begin(w.title, nullptr, flags);
    ws.pos = ImGui::GetWindowPos();
    ws.hasPos = true;
    drawShadow(ws.pos, ImGui::GetWindowSize());

    int enabled = 0;
    for (auto& e : w.entries) {
        if (e.ptr && *e.ptr) enabled++;
    }
    titleBar(w, ws, enabled);

    if (ws.openT > 0.001f) {
        if (ImGui::BeginChild("##content", ImVec2(0.f, 0.f), ImGuiChildFlags_None, ImGuiWindowFlags_NoBackground)) {
            extraRow(w.extra);
            for (auto& e : w.entries) {
                bool pressed = toggleRow(e, w.centered);
                // the fps toggle needs to apply immediately
                if (pressed && e.ptr == &cat::state().fpsEnabled) applyFps();
            }
        }
        ImGui::EndChild();
    }
    ImGui::End();
}

void drawFrame() {
    static bool first = true;
    if (first) {
        first = false;
        if (cat::state().fpsEnabled) applyFps();
    }

    ImGuiIO& io = ImGui::GetIO();
    bool viewportChanged = g_savedViewportWidth <= 0.f || g_savedViewportHeight <= 0.f ||
        std::abs(g_savedViewportWidth - io.DisplaySize.x) > 1.f ||
        std::abs(g_savedViewportHeight - io.DisplaySize.y) > 1.f;
    bool scaleChanged = g_savedLayoutScale <= 0.f || std::abs(g_savedLayoutScale - g_scale) > 0.001f;
    if (g_resetWindowPositions || viewportChanged || scaleChanged) {
        for (auto& w : cat::layout()) {
            g_win[w.title].hasPos = false;
        }
        g_savedViewportWidth = io.DisplaySize.x;
        g_savedViewportHeight = io.DisplaySize.y;
        g_savedLayoutScale = g_scale;
        g_resetWindowPositions = false;
        saveConfig();
    }

    g_fade = approach(g_fade, g_open ? 1.f : 0.f, io.DeltaTime, kFadeAnim);
    if (g_fade <= 0.f && !g_open) return;

#if IMGUI_VERSION_NUM >= 19200
    ImGui::GetStyle().FontScaleMain = g_scale;
#else
    io.FontGlobalScale = g_scale;
#endif

    float rowH = rowHeight();
    constexpr float margin = 4.f;
    constexpr float colGap = 4.f;
    constexpr float stackGap = 2.f;
    constexpr float rowGap = 4.f;
    constexpr int kMaxColumns = 16;

    int columnWindows[kMaxColumns] = {};
    int logicalColumnCount = 0;
    for (auto& w : cat::layout()) {
        int col = std::clamp(w.column, 0, kMaxColumns - 1);
        ++columnWindows[col];
        logicalColumnCount = std::max(logicalColumnCount, col + 1);
    }
    if (logicalColumnCount == 0) return;

    float availableW = std::max(1.f, io.DisplaySize.x - 2.f * margin);
    float desiredWidth = std::max(124.f, std::floor(ImGui::GetFontSize() * 12.f));
    int columnsPerRow = std::clamp(
        static_cast<int>((availableW + colGap) / (desiredWidth + colGap)),
        1,
        logicalColumnCount
    );
    if (g_savedColumnsPerRow != columnsPerRow) {
        for (auto& w : cat::layout()) {
            g_win[w.title].hasPos = false;
        }
        g_savedColumnsPerRow = columnsPerRow;
        saveConfig();
    }
    float width = std::min(
        desiredWidth,
        (availableW - colGap * static_cast<float>(columnsPerRow - 1)) / static_cast<float>(columnsPerRow)
    );

    int rowCount = (logicalColumnCount + columnsPerRow - 1) / columnsPerRow;
    float availableH = std::max(rowH + 2.f, io.DisplaySize.y - 2.f * margin);
    float rowBudget = std::max(
        rowH + 2.f,
        (availableH - rowGap * static_cast<float>(rowCount - 1)) / static_cast<float>(rowCount)
    );

    float rowStartX[kMaxColumns] = {};
    for (int row = 0; row < rowCount; ++row) {
        int rowColumns = std::min(columnsPerRow, logicalColumnCount - row * columnsPerRow);
        float gridWidth = width * static_cast<float>(rowColumns) + colGap * static_cast<float>(rowColumns - 1);
        rowStartX[row] = margin + std::max(0.f, (availableW - gridWidth) * 0.5f);
    }

    float colY[kMaxColumns] = {};
    int colWindowsRemaining[kMaxColumns] = {};
    for (int col = 0; col < logicalColumnCount; ++col) {
        colWindowsRemaining[col] = columnWindows[col];
    }

    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, std::max(ease(g_fade), 0.01f));

    for (auto& w : cat::layout()) {
        int col = std::clamp(w.column, 0, kMaxColumns - 1);
        int gridRow = col / columnsPerRow;
        int gridColumn = col % columnsPerRow;
        auto& ws = g_win[w.title];
        int windowsRemaining = std::max(1, colWindowsRemaining[col]);
        float remainingHeight = rowBudget - colY[col] - stackGap * static_cast<float>(windowsRemaining - 1);
        float maxHeight = std::max(rowH + 2.f, remainingHeight / static_cast<float>(windowsRemaining));
        ImVec2 defPos(
            rowStartX[gridRow] + static_cast<float>(gridColumn) * (width + colGap),
            margin + static_cast<float>(gridRow) * (rowBudget + rowGap) + colY[col]
        );

        if (ws.hasPos && (ws.pos.x < 0.f || ws.pos.y < 0.f ||
            ws.pos.x + width > io.DisplaySize.x || ws.pos.y + maxHeight > io.DisplaySize.y)) {
            ws.hasPos = false;
        }
        float height = windowHeight(w, ws, rowH, maxHeight);
        colY[col] += height;
        if (windowsRemaining > 1) colY[col] += stackGap;
        --colWindowsRemaining[col];
        drawWindow(w, defPos, width, rowH, maxHeight);
    }

    ImGui::PopStyleVar();
}

} // namespace

$on_mod(Loaded) {
    loadConfig();

    ImGuiCocos::get()
        .setup([] {
            // Reapply the theme and bundled font whenever ImGui's context is
            // initialized or reloaded (for example, after toggling fullscreen).
            applyTheme();
        })
        .draw([] {
            drawFrame();
        });
}

// ImGui only receives keys while it wants to capture keyboard input. That
// means a Tab press can't open a closed menu from inside drawFrame; listen to
// Geode's global keyboard event so the hotkey works in both states.
$execute {
    KeyboardInputEvent().listen([](KeyboardInputData& event) {
        if (event.key != KEY_Tab || event.action != KeyboardInputData::Action::Press) {
            return ListenerResult::Propagate;
        }

        auto& imgui = ImGuiCocos::get();
        if (imgui.isInitialized() && ImGui::GetIO().WantTextInput) {
            return ListenerResult::Propagate;
        }

        setOpen(!g_open);
        return ListenerResult::Stop;
    }).leak();
}
