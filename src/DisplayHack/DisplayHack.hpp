#pragma once

#include <string_view>

namespace cat::display {

// GD's native ForceTimer and VSync controls are exposed only on these targets
// by the 2.2081 bindings. Android has no supported dynamic setter in that API.
#if defined(GEODE_IS_WINDOWS) || defined(GEODE_IS_MACOS) || defined(GEODE_IS_IOS)
inline constexpr bool kHzBypassSupported = true;
inline constexpr bool kVerticalSyncSupported = true;
#else
inline constexpr bool kHzBypassSupported = false;
inline constexpr bool kVerticalSyncSupported = false;
#endif

// Gameplay update substepping is implemented from GJBaseGameLayer::update on
// every configured target. The render-layer visit hook required for visual-only
// extrapolation is bound on Windows/macOS/iOS, but not Android in GD 2.2081.
inline constexpr bool kTpsBypassSupported = true;
#if defined(GEODE_IS_WINDOWS) || defined(GEODE_IS_MACOS) || defined(GEODE_IS_IOS)
inline constexpr bool kFrameExtrapolationSupported = true;
#else
inline constexpr bool kFrameExtrapolationSupported = false;
#endif

struct Settings {
    bool fpsBypass = false;
    double fpsTarget = 240.0;

    bool tpsBypass = false;
    double tpsTarget = 240.0;

    bool hzBypass = false;
    bool frameExtrapolation = false;
    bool verticalSync = false;
};

Settings& settings();

// Configuration is loaded and saved by the menu, but display-specific keys
// and native settings are owned here.
void loadConfig();
void saveConfig();

// Called from the shared per-frame ImGui draw callback. It applies saved
// display preferences after GameManager/CCApplication are available.
void onFrame();
void onRenderContextReinitialized();
void onVideoSettingsLoaded(bool nativeForceTimer, bool nativeVSync);
void onToggle(std::string_view id);
void resetGameplayState();

// Display category controls rendered in the shared menu window.
void drawControls();
int controlsRowCount();

namespace fps {
bool apply();
void recordRenderCallback();
double measuredRate();
bool targetIsSafe();
}

namespace tps {
void resetAccumulator();
void recordGameplayUpdate();
double measuredRate();
bool targetIsSafe();
bool workloadLimited();
bool largeDeltaFallback();
const char* statusText();
}

namespace hz {
void apply(bool enabled);
}

namespace extrapolation {
void resetHistory();
}

namespace vsync {
void apply(bool enabled);
}

} // namespace cat::display
