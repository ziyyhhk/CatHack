#include "DisplayHack.hpp"

#include "../Hacks.hpp"

#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>

using namespace geode::prelude;

namespace cat::display::tps {
namespace {
using Clock = std::chrono::steady_clock;

// update(float) takes a float delta. Below 1 microsecond, a requested step is
// not useful enough to justify claiming the target can be safely applied.
constexpr double kMinimumStepSeconds = 0.000001;
constexpr double kMaximumStepSeconds = 0.25;
constexpr std::size_t kMaxSubstepsPerOuterUpdate = 512;
constexpr float kLargeOuterDeltaSeconds = 0.25f;

long double g_accumulator = 0.0L;
GJBaseGameLayer* g_accumulatorLayer = nullptr;
Clock::time_point g_measureStart{};
unsigned long long g_updateCalls = 0;
double g_measuredRate = 0.0;
bool g_measureStarted = false;
bool g_windowBudgetLimited = false;
bool g_windowLargeDeltaFallback = false;
bool g_recentBudgetLimited = false;
bool g_recentLargeDeltaFallback = false;

void noteBudgetLimit() {
    g_windowBudgetLimited = true;
}

void noteLargeDeltaFallback() {
    g_windowLargeDeltaFallback = true;
}
}

bool targetIsSafe() {
    const double target = settings().tpsTarget;
    if (!std::isfinite(target) || target <= 0.0) return false;
    const double step = 1.0 / target;
    return std::isfinite(step) && step >= kMinimumStepSeconds && step <= kMaximumStepSeconds &&
        std::isfinite(static_cast<float>(step)) && static_cast<float>(step) > 0.0f;
}

void resetAccumulator() {
    g_accumulator = 0.0L;
    g_accumulatorLayer = nullptr;
}

void recordGameplayUpdate() {
    const auto now = Clock::now();
    if (!g_measureStarted) {
        g_measureStart = now;
        g_measureStarted = true;
    }

    ++g_updateCalls;
    const double elapsed = std::chrono::duration<double>(now - g_measureStart).count();
    if (elapsed >= 0.5) {
        g_measuredRate = static_cast<double>(g_updateCalls) / elapsed;
        g_updateCalls = 0;
        g_measureStart = now;
        g_recentBudgetLimited = g_windowBudgetLimited;
        g_recentLargeDeltaFallback = g_windowLargeDeltaFallback;
        g_windowBudgetLimited = false;
        g_windowLargeDeltaFallback = false;
    }
}

double measuredRate() {
    return g_measuredRate;
}

bool workloadLimited() {
    return g_recentBudgetLimited || g_windowBudgetLimited;
}

bool largeDeltaFallback() {
    return g_recentLargeDeltaFallback || g_windowLargeDeltaFallback;
}

const char* statusText() {
    if (!settings().tpsBypass) return "TPS bypass off";
    if (!targetIsSafe()) return "target not applied: unsafe step size";
    if (workloadLimited()) return "512-step work budget reached; target may lag";
    if (largeDeltaFallback()) return "large delta used engine fallback";
    return "substeps active; compare measured rate to target";
}

} // namespace cat::display::tps

// Run the actual gameplay layer update at the requested fixed-step cadence.
// The bounded work budget prevents unbounded catch-up; under overload it uses
// fewer, wider substeps to preserve elapsed simulation time and exposes the
// measured update rate instead of claiming the requested target was achieved.
class $modify(CatHackTpsBypass, GJBaseGameLayer) {
    void update(float dt) {
        auto* manager = GameManager::get();
        auto* playLayer = manager ? manager->m_playLayer : nullptr;
        const bool isActivePlayLayer = playLayer && this == playLayer;

        if (!isActivePlayLayer) {
            cat::display::tps::resetAccumulator();
            GJBaseGameLayer::update(dt);
            return;
        }

        auto runOne = [&](float step) {
            GJBaseGameLayer::update(step);
            cat::display::tps::recordGameplayUpdate();
        };

        if (playLayer->m_isPaused) {
            cat::display::tps::resetAccumulator();
            GJBaseGameLayer::update(dt);
            return;
        }

        auto& display = cat::display::settings();
        if (!display.tpsBypass) {
            cat::display::tps::resetAccumulator();
            runOne(dt);
            return;
        }

        if (cat::display::tps::targetIsSafe() && display.tpsTarget != 240.0) {
            cat::state().cheated = true;
        }

        if (!cat::display::tps::targetIsSafe() || this->m_playerDied ||
            !std::isfinite(dt) || dt <= 0.0f) {
            cat::display::tps::resetAccumulator();
            runOne(dt);
            return;
        }

        if (dt > cat::display::tps::kLargeOuterDeltaSeconds) {
            cat::display::tps::resetAccumulator();
            cat::display::tps::noteLargeDeltaFallback();
            runOne(dt);
            return;
        }

        if (cat::display::tps::g_accumulatorLayer != this) {
            cat::display::tps::g_accumulator = 0.0L;
            cat::display::tps::g_accumulatorLayer = this;
        }

        const long double quantum = 1.0L / static_cast<long double>(display.tpsTarget);
        const long double total = cat::display::tps::g_accumulator + static_cast<long double>(dt);
        const long double due = std::floor(total / quantum);
        if (!std::isfinite(due) || due < 1.0L) {
            cat::display::tps::g_accumulator = total;
            return;
        }

        const bool budgetLimited = due > static_cast<long double>(cat::display::tps::kMaxSubstepsPerOuterUpdate);
        const std::size_t substepCount = budgetLimited
            ? cat::display::tps::kMaxSubstepsPerOuterUpdate
            : static_cast<std::size_t>(due);
        if (budgetLimited) {
            // Maintain game-time progress without an unbounded loop. The
            // observed TPS metric will show that this workload did not meet
            // the user's requested target.
            cat::display::tps::g_accumulator = 0.0L;
            cat::display::tps::noteBudgetLimit();
        } else {
            cat::display::tps::g_accumulator = std::max(
                0.0L,
                total - static_cast<long double>(substepCount) * quantum
            );
        }

        const long double substepDelta = budgetLimited
            ? total / static_cast<long double>(substepCount)
            : quantum;
        const float step = static_cast<float>(substepDelta);
        if (!std::isfinite(step) || step <= 0.0f) {
            cat::display::tps::resetAccumulator();
            cat::display::tps::noteLargeDeltaFallback();
            runOne(dt);
            return;
        }

        for (std::size_t index = 0; index < substepCount; ++index) {
            // If death, pause, or a completion transition begins mid-batch,
            // stop stepping immediately rather than running stale catch-up.
            if (playLayer->m_isPaused || this->m_playerDied) {
                cat::display::tps::resetAccumulator();
                break;
            }
            runOne(step);
        }
    }
};
