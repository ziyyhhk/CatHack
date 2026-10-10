#include "DisplayHack.hpp"

#include <Geode/Geode.hpp>

using namespace geode::prelude;

namespace cat::display::hz {

void apply(bool enabled) {
#if defined(GEODE_IS_WINDOWS) || defined(GEODE_IS_MACOS) || defined(GEODE_IS_IOS)
    // Use Geometry Dash's native ForceTimer wrapper. The binding proves this
    // is a timer control, not a monitor refresh-rate setter; its exact effect
    // is left to GD's platform implementation.
    PlatformToolbox::toggleForceTimer(enabled);
#else
    // The 2.2081 Android bindings do not expose this native dynamic setter.
    (void)enabled;
#endif
}

} // namespace cat::display::hz
