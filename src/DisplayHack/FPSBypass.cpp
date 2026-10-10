#include "DisplayHack.hpp"

#include <Geode/Geode.hpp>

#include <chrono>
#include <cmath>

using namespace geode::prelude;

namespace cat::display::fps {
namespace {
using Clock = std::chrono::steady_clock;
Clock::time_point g_windowStart{};
unsigned long long g_windowCalls = 0;
double g_measuredRate = 0.0;
bool g_windowStarted = false;

// CCApplication accepts a double, but intervals near zero are not useful to
// the engine and can underflow in platform timer implementations. Refuse such
// requests explicitly rather than silently clamping the user's target.
constexpr double kMinimumSafeInterval = 0.000001; // 1 MHz upper timer interval
constexpr double kMaximumSafeInterval = 60.0;     // avoid a multi-minute stall
}

bool targetIsSafe() {
    const double target = settings().fpsTarget;
    if (!std::isfinite(target) || target <= 0.0) return false;
    const double interval = 1.0 / target;
    return std::isfinite(interval) &&
        interval >= kMinimumSafeInterval &&
        interval <= kMaximumSafeInterval;
}

bool apply() {
    auto* application = cocos2d::CCApplication::sharedApplication();
    if (!application) return false;

    double interval = 1.0 / 60.0;
    if (settings().fpsBypass) {
        if (!targetIsSafe()) return false;
        interval = 1.0 / settings().fpsTarget;
    }
    application->setAnimationInterval(interval);
    return true;
}

void recordRenderCallback() {
    const auto now = Clock::now();
    if (!g_windowStarted) {
        g_windowStart = now;
        g_windowStarted = true;
    }

    ++g_windowCalls;
    const double elapsed = std::chrono::duration<double>(now - g_windowStart).count();
    if (elapsed >= 0.5) {
        g_measuredRate = static_cast<double>(g_windowCalls) / elapsed;
        g_windowCalls = 0;
        g_windowStart = now;
    }
}

double measuredRate() {
    return g_measuredRate;
}

} // namespace cat::display::fps
