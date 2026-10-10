#include "DisplayHack.hpp"

#include <Geode/Geode.hpp>

using namespace geode::prelude;

namespace cat::display::vsync {

void apply(bool enabled) {
#if defined(GEODE_IS_WINDOWS) || defined(GEODE_IS_MACOS) || defined(GEODE_IS_IOS)
    // This is GD's native VSync control, separate from FPS interval and TPS.
    PlatformToolbox::toggleVerticalSync(enabled);
#else
    // The 2.2081 Android bindings do not expose this dynamic control.
    (void)enabled;
#endif
}

} // namespace cat::display::vsync

// macOS and iOS have a hookable native settings reload. Windows marks this
// 2.2081 function inline, so its transitions cannot be intercepted here.
#if defined(GEODE_IS_MACOS) || defined(GEODE_IS_IOS)
#include <Geode/modify/GameManager.hpp>

class $modify(CatHackDisplayVideoSettingsReload, GameManager) {
    void loadVideoSettings() {
        GameManager::loadVideoSettings();
        cat::display::onVideoSettingsLoaded(
            this->getGameVariable(GameVar::ForceTimer),
            this->getGameVariable(GameVar::VerticalSync)
        );
    }
};
#endif
