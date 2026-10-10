#pragma once

#include "DisplayHack/DisplayHack.hpp"

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

    float speed = 1.f;
    float interfaceScale = 0.8f;

    // runtime only (not saved): true once a cheat touched the current level
    bool cheated = false;
};

State& state();

// One row in a window. ptr == nullptr means the row is not supported (drawn
// dimmed and non-interactive); supported display hacks own their own state.
struct Entry {
    const char* label;
    bool* ptr = nullptr;
    const char* id = nullptr;   // stable UI/save key
    const char* desc = nullptr; // shown in the hover tooltip
};

// Special input rows drawn at the top of a window.
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
