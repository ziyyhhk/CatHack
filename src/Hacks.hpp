#pragma once
#include <vector>

namespace cat {

// Everything that actually does something lives here.
struct State {
    bool noclip = false;
    bool speedEnabled = false;
    bool safeMode = false;
    bool autoSafeMode = true;
    bool noDeathEffect = false;
    bool unlockIcons = false;
    bool textLength = false;
    bool fpsEnabled = false;
    bool physicsTpsEnabled = false;
    bool verticalSyncEnabled = false;
    bool verticalSyncPreferenceSaved = false; // whether CatHack has an explicit saved VSync override
    bool verticalSyncInitialized = false; // runtime-only; set after native preference is applied
    bool displaySettingsLoaded = false; // runtime-only; set after saved display settings are read

    float speed = 1.f;
    double fps = 240.0;
    double physicsTps = 240.0;
    float interfaceScale = 0.8f;

    // runtime only (not saved): true once a cheat touched the current level
    bool cheated = false;
};

State& state();

// GD 2.2081 exposes a per-frame modified-delta hook, but no verified API for
// running additional gameplay updates independently from the render scheduler.
// Do not advertise the current delta hook as a true TPS bypass.
inline constexpr bool kPhysicsTpsSupported = false;

// PlatformToolbox exposes Geometry Dash's native VSync toggle on Windows,
// macOS and iOS. The 2.2081 Android bindings do not expose that control.
#if defined(GEODE_IS_WINDOWS) || defined(GEODE_IS_MACOS) || defined(GEODE_IS_IOS)
inline constexpr bool kVerticalSyncSupported = true;
#else
inline constexpr bool kVerticalSyncSupported = false;
#endif

void applyVerticalSync(bool enabled);
void initializeVerticalSync(bool originalValue, bool enabled);

// One row in a window. ptr == nullptr means "not implemented yet"
// (the row is drawn dimmed and can't be toggled).
struct Entry {
    const char* label;
    bool* ptr = nullptr;
    const char* id = nullptr;   // save key, only needed when ptr is set
    const char* desc = nullptr; // shown in the hover tooltip
};

// Special input row drawn at the top of a window.
enum class Extra { None, Scale, Speed, Display };

struct Window {
    const char* title;
    int column;      // which column the window starts in
    Extra extra;
    bool centered;   // centered text (action buttons) vs left aligned (toggles)
    std::vector<Entry> entries;
};

std::vector<Window> const& layout();

} // namespace cat
