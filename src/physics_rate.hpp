#pragma once

#include <algorithm>
#include <cmath>

namespace cat::detail {

struct FixedStepBatch {
    double consumed = 0.0;
    double remainder = 0.0;
};

// Consume all whole simulation steps available in elapsed time and preserve
// the fractional remainder for the next rendered frame.
inline FixedStepBatch consumeFixedSteps(double elapsed, double ticksPerSecond, double timeWarp) noexcept {
    if (!std::isfinite(elapsed) || !std::isfinite(ticksPerSecond) || !std::isfinite(timeWarp) ||
        elapsed < 0.0 || ticksPerSecond <= 0.0 || timeWarp <= 0.0) {
        return {};
    }

    const double step = std::min(timeWarp, 1.0) / ticksPerSecond;
    if (!std::isfinite(step) || step <= 0.0) return {};

    // The small tolerance avoids dropping a whole step due only to float
    // rounding at common rates such as 60, 120, and 240 Hz.
    const double wholeSteps = std::floor((elapsed / step) + 1e-9);
    const double consumed = std::min(elapsed, wholeSteps * step);
    const double remainder = std::max(0.0, elapsed - consumed);
    return {consumed, remainder};
}

} // namespace cat::detail
