#include "DisplayHack.hpp"

#include "../Hacks.hpp"

#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <imgui.h>
#include <imgui_internal.h>

#include <cmath>

using namespace geode::prelude;

namespace cat::display {
namespace {
Settings g_settings;
bool g_configLoaded = false;
bool g_nativePreferencesInitialized = false;
bool g_hzPreferenceSaved = false;
bool g_vsyncPreferenceSaved = false;
bool g_startupFpsApplied = false;
double g_lastValidFps = 240.0;
double g_lastValidTps = 240.0;
constexpr double kFpsWarningThreshold = 360.0;
constexpr double kTpsDefault = 240.0;

// Return the last valid numeric value during partial/invalid ImGui edits. The
// saved target itself remains an unrestricted positive finite double.
double sanitizeRate(double value, double fallback) {
    return std::isfinite(value) && value > 0.0 ? value : fallback;
}

void drawRateInput(
    const char* label,
    const char* id,
    double& value,
    double& lastValid,
    bool tpsTarget
) {
    ImGui::TextUnformatted(label);
    ImGui::SameLine(0.f, 4.f);
    ImGui::SetNextItemWidth(-1.f);
    ImGui::InputDouble(id, &value, 0.0, 0.0, "%.15g", ImGuiInputTextFlags_CharsScientific);

    const bool invalid = !std::isfinite(value) || value <= 0.0;
    if (invalid && !ImGui::IsItemActive()) {
        value = lastValid;
    } else if (!invalid) {
        lastValid = value;
    }

    if (ImGui::IsItemDeactivatedAfterEdit()) {
        if (invalid) value = lastValid > 0.0 ? lastValid : kTpsDefault;
        if (tpsTarget) {
            tps::resetAccumulator();
            if (g_settings.tpsBypass && value != kTpsDefault && tps::targetIsSafe()) {
                cat::state().cheated = true;
            }
        } else if (g_settings.fpsBypass) {
            fps::apply();
        }
        saveConfig();
    }

    if (ImGui::IsItemHovered() && ImGui::GetCurrentContext()->HoveredIdTimer > 0.45f) {
        ImGui::BeginTooltip();
        if (tpsTarget) {
            if (value != kTpsDefault) {
                ImGui::TextUnformatted("Warning: a non-240 TPS target marks the run as cheated while it is applied and may change physics/collisions.");
            }
            ImGui::TextUnformatted(
                "This is gameplay simulation cadence, separate from render FPS and monitor refresh. The engine update is split into bounded substeps; 3000 TPS is accepted without a 240 cap. Requested steps shorter than 1 microsecond or longer than 250 milliseconds are retained but not applied. At most 512 substeps run per outer update, so compare the measured update rate below with the requested target."
            );
        } else {
            if (value > kFpsWarningThreshold) {
                ImGui::TextUnformatted("Warning: very high render targets can increase load and may affect gameplay timing.");
            }
            ImGui::TextUnformatted(
                "Render-timer target only. Any positive finite value can be entered; intervals outside 1 microsecond to 60 seconds are retained but not applied rather than silently clamped. The observed frame-callback rate below is not a hardware-present or monitor-refresh measurement."
            );
        }
        ImGui::EndTooltip();
    }
}

} // namespace

Settings& settings() {
    return g_settings;
}

void loadConfig() {
    auto* mod = Mod::get();

    g_settings.fpsBypass = mod->getSavedValue<bool>("unlock-fps", false);
    g_settings.fpsTarget = sanitizeRate(mod->getSavedValue<double>("fps", 240.0), 240.0);
    g_settings.tpsBypass = mod->getSavedValue<bool>("physics-tps-enabled", false);
    g_settings.tpsTarget = sanitizeRate(mod->getSavedValue<double>("physics-tps", 240.0), 240.0);
    g_settings.frameExtrapolation = mod->getSavedValue<bool>("frame-extrapolation", false);

    g_hzPreferenceSaved = mod->hasSavedValue("hz-bypass");
    g_settings.hzBypass = g_hzPreferenceSaved
        ? mod->getSavedValue<bool>("hz-bypass", false)
        : false;

    g_vsyncPreferenceSaved = mod->hasSavedValue("vertical-sync");
    g_settings.verticalSync = g_vsyncPreferenceSaved
        ? mod->getSavedValue<bool>("vertical-sync", false)
        : false;

    g_lastValidFps = g_settings.fpsTarget;
    g_lastValidTps = g_settings.tpsTarget;
    g_configLoaded = true;
    g_nativePreferencesInitialized = false;
    g_startupFpsApplied = false;
}

void saveConfig() {
    auto* mod = Mod::get();
    g_settings.fpsTarget = sanitizeRate(g_settings.fpsTarget, g_lastValidFps);
    g_settings.tpsTarget = sanitizeRate(g_settings.tpsTarget, g_lastValidTps);
    g_lastValidFps = g_settings.fpsTarget;
    g_lastValidTps = g_settings.tpsTarget;

    mod->setSavedValue<bool>("unlock-fps", g_settings.fpsBypass);
    mod->setSavedValue<double>("fps", g_settings.fpsTarget);
    mod->setSavedValue<bool>("physics-tps-enabled", g_settings.tpsBypass);
    mod->setSavedValue<double>("physics-tps", g_settings.tpsTarget);
    mod->setSavedValue<bool>("frame-extrapolation", g_settings.frameExtrapolation);

    // If the user has never overridden either native preference, leave it
    // owned by Geometry Dash instead of turning the detected value into a
    // CatHack override merely because another setting was saved.
    if (g_hzPreferenceSaved) {
        mod->setSavedValue<bool>("hz-bypass", g_settings.hzBypass);
    }
    if (g_vsyncPreferenceSaved) {
        mod->setSavedValue<bool>("vertical-sync", g_settings.verticalSync);
    }
}

void onFrame() {
    if (!g_configLoaded) return;

    if (!g_nativePreferencesInitialized) {
        if (auto* manager = GameManager::get()) {
            onVideoSettingsLoaded(
                manager->getGameVariable(GameVar::ForceTimer),
                manager->getGameVariable(GameVar::VerticalSync)
            );
        }
    }

    if (!g_startupFpsApplied) {
        if (!g_settings.fpsBypass) {
            g_startupFpsApplied = true;
        } else {
            g_startupFpsApplied = fps::apply();
        }
    }
}

void onRenderContextReinitialized() {
    if (!g_configLoaded) return;
    g_nativePreferencesInitialized = false;
    g_startupFpsApplied = false;
}

void onVideoSettingsLoaded(bool nativeForceTimer, bool nativeVSync) {
    if (!g_configLoaded) return;

    if (!g_hzPreferenceSaved) {
        g_settings.hzBypass = nativeForceTimer;
    } else {
        hz::apply(g_settings.hzBypass);
    }

    if (!g_vsyncPreferenceSaved) {
        g_settings.verticalSync = nativeVSync;
    } else {
        vsync::apply(g_settings.verticalSync);
    }
    g_nativePreferencesInitialized = true;
}

void onToggle(std::string_view id) {
    if (id == "unlock-fps") {
        g_startupFpsApplied = fps::apply();
    } else if (id == "physics-tps-enabled") {
        tps::resetAccumulator();
        if (g_settings.tpsBypass && g_settings.tpsTarget != kTpsDefault && tps::targetIsSafe()) {
            cat::state().cheated = true;
        }
    } else if (id == "hz-bypass") {
        g_hzPreferenceSaved = true;
        hz::apply(g_settings.hzBypass);
    } else if (id == "frame-extrapolation") {
        extrapolation::resetHistory();
    } else if (id == "vertical-sync") {
        g_vsyncPreferenceSaved = true;
        vsync::apply(g_settings.verticalSync);
    }
}

void resetGameplayState() {
    tps::resetAccumulator();
    extrapolation::resetHistory();
}

int controlsRowCount() {
    // Two independent numeric targets followed by measured render/game update
    // rates. Keep the Display window's existing auto-reflow aware of all rows.
    return 4;
}

void drawControls() {
    drawRateInput("FPS target", "##display-fps-target", g_settings.fpsTarget, g_lastValidFps, false);
    drawRateInput("TPS target", "##display-tps-target", g_settings.tpsTarget, g_lastValidTps, true);

    ImGui::Text("Frame callbacks/s: %.1f | %s",
        fps::measuredRate(),
        !g_settings.fpsBypass ? "FPS bypass off" :
            (fps::targetIsSafe() ? "FPS target requested" : "FPS target not applied: timer range"));

    ImGui::Text("Gameplay updates/s: %.1f | %s",
        tps::measuredRate(), tps::statusText());
}

} // namespace cat::display

// Shared lifecycle integration: reset fixed-step phase and visual history at
// pause/resume, restart, death flows that reset the level, and scene exit.
class $modify(DisplayHackPlayLayerLifecycle, PlayLayer) {
    void pauseGame(bool unfocused) {
        cat::display::resetGameplayState();
        PlayLayer::pauseGame(unfocused);
    }

    void resume() {
        cat::display::resetGameplayState();
        PlayLayer::resume();
    }

    void resetLevel() {
        cat::display::resetGameplayState();
        PlayLayer::resetLevel();
    }

    void resetLevelFromStart() {
        cat::display::resetGameplayState();
        PlayLayer::resetLevelFromStart();
    }

    void resumeAndRestart(bool fromStart) {
        cat::display::resetGameplayState();
        PlayLayer::resumeAndRestart(fromStart);
    }

    void onExit() {
        cat::display::resetGameplayState();
        PlayLayer::onExit();
    }
};
