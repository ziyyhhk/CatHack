#include "Hacks.hpp"

#include <Geode/Geode.hpp>

namespace cat {

void applyVerticalSync(bool enabled) {
#if defined(GEODE_IS_WINDOWS) || defined(GEODE_IS_MACOS) || defined(GEODE_IS_IOS)
    // Use the game's GD 2.2081 platform wrapper rather than calling a guessed
    // graphics API or changing the render interval used by FPS Bypass.
    PlatformToolbox::toggleVerticalSync(enabled);
#else
    (void)enabled;
#endif
    state().verticalSyncInitialized = true;
}

void initializeVerticalSync(bool originalValue, bool enabled) {
    auto& current = state();
    if (!current.verticalSyncPreferenceSaved) {
        // With no CatHack override, follow Geometry Dash's own setting instead
        // of pinning an early/default value read before its settings load.
        current.verticalSyncEnabled = originalValue;
        enabled = originalValue;
    }
    // On macOS/iOS this also follows subsequent hookable video-settings loads.
    current.verticalSyncInitialized = true;
    applyVerticalSync(enabled);
}

} // namespace cat
