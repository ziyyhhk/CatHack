#include "Hacks.hpp"

#include <Geode/Geode.hpp>
#include <imgui-cocos.hpp>
#include <imgui.h>

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstdio>
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

// ---------------------------------------------------------------- state
struct WinState {
    ImVec2 pos{0.f, 0.f};
    bool hasPos = false;
    bool collapsed = false;
    float openT = 1.f; // 1 = open, 0 = collapsed
};

bool g_open = false;
float g_fade = 0.f;
float g_scale = 1.2f; // applied scale (only updated after you let go of the slider)
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

    for (auto& [title, ws] : g_win) {
        mod->setSavedValue<bool>("collapsed-" + title, ws.collapsed);
        if (ws.hasPos) {
            mod->setSavedValue<double>("pos-" + title + "-x", ws.pos.x);
            mod->setSavedValue<double>("pos-" + title + "-y", ws.pos.y);
        }
    }
}

void loadConfig() {
    auto* mod = Mod::get();
    auto& s = cat::state();

    for (auto& w : cat::layout()) {
        for (auto& e : w.entries) {
            if (e.ptr && e.id) *e.ptr = mod->getSavedValue<bool>(e.id, *e.ptr);
        }

        std::string title = w.title;
        auto& ws = g_win[title];
        ws.collapsed = mod->getSavedValue<bool>("collapsed-" + title, false);
        ws.openT = ws.collapsed ? 0.f : 1.f;
        double x = mod->getSavedValue<double>("pos-" + title + "-x", -1.0);
        double y = mod->getSavedValue<double>("pos-" + title + "-y", -1.0);
        if (x >= 0.0 && y >= 0.0) {
            ws.pos = ImVec2(static_cast<float>(x), static_cast<float>(y));
            ws.hasPos = true;
        }
    }

    s.speed = static_cast<float>(mod->getSavedValue<double>("speed", s.speed));
    s.fps = static_cast<float>(mod->getSavedValue<double>("fps", s.fps));
    s.interfaceScale = static_cast<float>(mod->getSavedValue<double>("interface-scale", s.interfaceScale));
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
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 4.f;
    style.WindowBorderSize = 0.f;
    style.WindowPadding = ImVec2(0.f, 0.f);
    style.FramePadding = ImVec2(6.f, 3.f);
    style.ItemSpacing = ImVec2(0.f, 0.f);
    style.ScrollbarSize = 8.f;

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
    ImVec2 bMin(p.x + 4.f, p.y + (h - btn) * 0.5f);
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
    dl->AddText(ImVec2(p.x + btn + 8.f, p.y + (h - ts.y) * 0.5f), withAlpha(kWhite), titleBuf);

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
    float trackW = 3.f;
    ImU32 trackCol = mix(withAlpha(kTrack), withAlpha(kPink), enT);
    dl->AddRectFilled(ImVec2(p.x + size.x - trackW, p.y), ImVec2(p.x + size.x, p.y + size.y), trackCol);

    ImU32 textCol = implemented ? mix(withAlpha(kText), withAlpha(kPink), enT * 0.6f) : withAlpha(kDim);
    ImVec2 ts = ImGui::CalcTextSize(e.label);
    float tx = centered ? p.x + (size.x - ts.x) * 0.5f : p.x + 8.f;
    dl->AddText(ImVec2(tx, p.y + (h - ts.y) * 0.5f), textCol, e.label);

    if (hovered && e.desc && ImGui::GetCurrentContext()->HoveredIdTimer > 0.45f) {
        ImGui::BeginTooltip();
        ImGui::TextUnformatted(e.desc);
        ImGui::EndTooltip();
    } else if (hovered && !implemented) {
        ImGui::BeginTooltip();
        ImGui::TextUnformatted("Not implemented yet");
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
        if (ImGui::SliderFloat("##scale", &v, 0.7f, 2.0f, "Scale %.2f")) {
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

void drawWindow(cat::Window const& w, ImVec2 defPos, float width, float rowH) {
    auto& ws = g_win[w.title];
    if (!ws.hasPos) {
        ws.pos = defPos;
        ws.hasPos = true;
    }

    float openH = rowH * (1.f + static_cast<float>(w.entries.size()) + (w.extra != cat::Extra::None ? 1.f : 0.f)) + 2.f;
    float height = rowH + (openH - rowH) * ws.openT;

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
        extraRow(w.extra);
        for (auto& e : w.entries) {
            bool pressed = toggleRow(e, w.centered);
            // the fps toggle needs to apply immediately
            if (pressed && e.ptr == &cat::state().fpsEnabled) applyFps();
        }
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
    if (!io.WantTextInput && ImGui::IsKeyPressed(ImGuiKey_Tab, false)) {
        setOpen(!g_open);
    }

    g_fade = approach(g_fade, g_open ? 1.f : 0.f, io.DeltaTime, kFadeAnim);
    if (g_fade <= 0.f && !g_open) return;

#if IMGUI_VERSION_NUM >= 19200
    ImGui::GetStyle().FontScaleMain = g_scale;
#else
    io.FontGlobalScale = g_scale;
#endif

    // size everything from the font so it follows the interface scale
    float rowH = rowHeight();
    float width = std::floor(ImGui::GetFontSize() * 15.f);
    float colGap = 4.f;
    float stackGap = 2.f;
    float colY[16] = {};

    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, std::max(ease(g_fade), 0.01f));

    for (auto& w : cat::layout()) {
        int rows = static_cast<int>(w.entries.size()) + (w.extra != cat::Extra::None ? 1 : 0);
        float height = rowH * static_cast<float>(rows + 1) + 2.f;
        int col = std::clamp(w.column, 0, 15);

        ImVec2 defPos(colGap + static_cast<float>(col) * (width + colGap), colGap + colY[col]);
        colY[col] += height + stackGap;

        drawWindow(w, defPos, width, rowH);
    }

    ImGui::PopStyleVar();
}

} // namespace

$on_mod(Loaded) {
    loadConfig();

    ImGuiCocos::get()
        .setup([] {
            // imgui gets re-initialised (e.g. when toggling fullscreen), so the
            // theme is applied here. Load a custom font here too if you want.
            applyTheme();
        })
        .draw([] {
            drawFrame();
        });
}
