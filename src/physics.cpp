#include "Hacks.hpp"
#include "physics_rate.hpp"

#include <Geode/Geode.hpp>

#if !defined(GEODE_IS_MACOS)
#include <Geode/modify/GJBaseGameLayer.hpp>

#include <algorithm>
#include <cmath>

using namespace geode::prelude;

namespace {
constexpr double kDefaultRenderFps = 60.0;
constexpr double kDefaultPhysicsTps = 240.0;
constexpr double kMinRate = 1.0;
constexpr double kMaxRate = 1000.0;

double boundedRate(double value, double fallback) {
    if (!std::isfinite(value)) return fallback;
    return std::clamp(value, kMinRate, kMaxRate);
}

double configuredPhysicsRate(cat::State const& state, float frameDelta) {
    double actualRenderFps = kDefaultRenderFps;
    if (std::isfinite(frameDelta) && frameDelta > 0.f) {
        actualRenderFps = 1.0 / static_cast<double>(frameDelta);

        // CCScheduler's speedhack scales the update delta. Undo that scale
        // when estimating the render cadence from this frame's delta.
        if (state.speedEnabled && std::isfinite(state.speed) && state.speed > 0.f) {
            actualRenderFps *= static_cast<double>(state.speed);
        }
    }

    double requestedRenderFps = state.fpsEnabled
        ? boundedRate(state.fps, kDefaultRenderFps)
        : kDefaultRenderFps;
    requestedRenderFps = std::max(requestedRenderFps, boundedRate(actualRenderFps, kDefaultRenderFps));

    const double requestedPhysicsTps = boundedRate(state.physicsTps, kDefaultPhysicsTps);
    return std::clamp(std::max(requestedRenderFps, requestedPhysicsTps), kMinRate, kMaxRate);
}
} // namespace

// GD 2.2081 normally quantizes gameplay time to its 240 Hz physics interval.
// Replacing this per-layer delta keeps the existing update loop but lets it
// consume a user-selected fixed-step rate while carrying fractional time
// forward. The game still uses its 2.2 physics model; this is not a 2.1 port.
class $modify(CatPhysicsTickRateLayer, GJBaseGameLayer) {
    double getModifiedDelta(float frameDelta) {
        auto& state = cat::state();
        if (!state.physicsTpsEnabled || !m_started || m_playerDied ||
            !std::isfinite(frameDelta) || frameDelta < 0.f) {
            return GJBaseGameLayer::getModifiedDelta(frameDelta);
        }

        const double timeWarp = static_cast<double>(m_gameState.m_timeWarp);
        if (!std::isfinite(timeWarp) || timeWarp <= 0.0) {
            return GJBaseGameLayer::getModifiedDelta(frameDelta);
        }

        const double targetTps = configuredPhysicsRate(state, frameDelta);
        if (std::abs(targetTps - kDefaultPhysicsTps) < 1e-6) {
            return GJBaseGameLayer::getModifiedDelta(frameDelta);
        }

        double elapsed = m_extraDelta;
        if (m_resumeTimer < 1) {
            elapsed += static_cast<double>(frameDelta);
        } else {
            --m_resumeTimer;
        }
        if (!std::isfinite(elapsed) || elapsed < 0.0) {
            return GJBaseGameLayer::getModifiedDelta(frameDelta);
        }

        auto batch = cat::detail::consumeFixedSteps(elapsed, targetTps, timeWarp);
        m_extraDelta = batch.remainder;
        return batch.consumed;
    }
};
#endif
