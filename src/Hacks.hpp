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

    float speed = 1.f;
    float fps = 240.f;
    float physicsTps = 240.f;
    float interfaceScale = 0.8f;

    // runtime only (not saved): true once a cheat touched the current level
    bool cheated = false;
};

State& state();

// GD 2.2081's macOS build does not expose a reliable hook for its modified
// physics delta. FPS targeting still works there; the independent TPS hook does not.
#if defined(GEODE_IS_MACOS)
inline constexpr bool kPhysicsTpsSupported = false;
#else
inline constexpr bool kPhysicsTpsSupported = true;
#endif

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
