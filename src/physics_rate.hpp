#pragma once

#include <algorithm>
#include <cmath>

namespace cat::detail {

constexpr double kMaxPhysicsStepsPerRenderedUpdate = 4096.0;
constexpr double kMaxPhysicsBacklogSeconds = 0.25;

struct FixedStepBatch {
    double consumed = 0.0;
    double remainder = 0.0;
};

// Consume whole simulation steps available in elapsed time, subject to a
// per-frame safety budget, and preserve the remainder for the next frame.
inline FixedStepBatch consumeFixedSteps(double elapsed, double ticksPerSecond, double timeWarp) noexcept {
    if (!std::isfinite(elapsed) || !std::isfinite(ticksPerSecond) || !std::isfinite(timeWarp) ||
        elapsed < 0.0 || ticksPerSecond <= 0.0 || timeWarp <= 0.0) {
        return {};
    }

    const double step = std::min(timeWarp, 1.0) / ticksPerSecond;
    if (!std::isfinite(step) || step <= 0.0) return {};

    // The small tolerance avoids dropping a whole step due only to float
    // rounding at common rates such as 60, 120, and 240 Hz.
    const double requestedSteps = std::floor((elapsed / step) + 1e-9);
    const bool overBudget = requestedSteps > kMaxPhysicsStepsPerRenderedUpdate;
    const double wholeSteps = std::min(requestedSteps, kMaxPhysicsStepsPerRenderedUpdate);
    const double consumed = std::min(elapsed, wholeSteps * step);
    double remainder = std::max(0.0, elapsed - consumed);
    if (overBudget) {
        // Do not let an extreme requested rate build an unbounded catch-up queue.
        remainder = std::min(remainder, kMaxPhysicsBacklogSeconds);
    }
    return {consumed, remainder};
}

} // namespace cat::detail
