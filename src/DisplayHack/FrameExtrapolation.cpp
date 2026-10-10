#include "DisplayHack.hpp"

#include <Geode/Geode.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <unordered_map>

using namespace geode::prelude;

namespace cat::display::extrapolation {
namespace {
using Clock = std::chrono::steady_clock;

struct PlayerHistory {
    cocos2d::CCPoint previousPosition{};
    cocos2d::CCPoint currentPosition{};
    Clock::time_point previousTimestamp{};
    Clock::time_point currentTimestamp{};
    float currentSimulationStep = 0.0f;
    bool valid = false;
};

std::unordered_map<PlayerObject*, PlayerHistory> g_history;
constexpr float kMaximumHistoryGap = 0.25f;
constexpr float kMaximumSimulationStep = 0.25f;
constexpr double kMaximumSegmentDistance = 512.0;
constexpr double kMaximumEstimatedSpeed = 30000.0;
constexpr double kMaximumPredictionAge = 1.0 / 30.0;

bool isCurrentGameplayPlayer(PlayerObject* player) {
    auto* manager = GameManager::get();
    auto* playLayer = manager ? manager->m_playLayer : nullptr;
    return player && playLayer && player->m_gameLayer == playLayer;
}

void recordSimulationState(PlayerObject* player, float dt) {
    if (!settings().frameExtrapolation || !isCurrentGameplayPlayer(player) ||
        player->m_isDead || player->m_wasTeleported || !std::isfinite(dt) ||
        dt <= 0.0f || dt > kMaximumSimulationStep) {
        g_history.erase(player);
        return;
    }

    const auto now = Clock::now();
    const cocos2d::CCPoint position = player->m_position;
    auto& history = g_history[player];
    if (!history.valid) {
        history.previousPosition = position;
        history.currentPosition = position;
        history.previousTimestamp = now;
        history.currentTimestamp = now;
        history.currentSimulationStep = dt;
        history.valid = true;
        return;
    }

    const double wallGap = std::chrono::duration<double>(now - history.currentTimestamp).count();
    const double dx = static_cast<double>(position.x) - history.currentPosition.x;
    const double dy = static_cast<double>(position.y) - history.currentPosition.y;
    const double segmentDistance = std::hypot(dx, dy);
    const double estimatedSpeed = segmentDistance / static_cast<double>(dt);
    if (!std::isfinite(wallGap) || wallGap < 0.0 || wallGap > kMaximumHistoryGap ||
        !std::isfinite(segmentDistance) || segmentDistance > kMaximumSegmentDistance ||
        !std::isfinite(estimatedSpeed) || estimatedSpeed > kMaximumEstimatedSpeed) {
        // A long stall, checkpoint/teleport, or discontinuous movement starts
        // a fresh history instead of extrapolating from stale coordinates.
        history.previousPosition = position;
        history.currentPosition = position;
        history.previousTimestamp = now;
        history.currentTimestamp = now;
        history.currentSimulationStep = dt;
        history.valid = true;
        return;
    }

    history.previousPosition = history.currentPosition;
    history.previousTimestamp = history.currentTimestamp;
    history.currentPosition = position;
    history.currentTimestamp = now;
    history.currentSimulationStep = dt;
}

bool predictedOffset(PlayerObject* player, cocos2d::CCPoint& offset) {
    auto found = g_history.find(player);
    if (found == g_history.end() || !found->second.valid) return false;

    auto& history = found->second;
    if (!settings().frameExtrapolation || !isCurrentGameplayPlayer(player) ||
        player->m_isDead || player->m_wasTeleported) {
        history.valid = false;
        return false;
    }

    const double step = static_cast<double>(history.currentSimulationStep);
    if (!std::isfinite(step) || step <= 0.0) return false;

    const auto now = Clock::now();
    const double age = std::chrono::duration<double>(now - history.currentTimestamp).count();
    const double sampleSpan = std::chrono::duration<double>(
        history.currentTimestamp - history.previousTimestamp
    ).count();
    if (!std::isfinite(age) || age < 0.0 || age > kMaximumHistoryGap ||
        !std::isfinite(sampleSpan) || sampleSpan < 0.0 || sampleSpan > kMaximumHistoryGap) {
        history.valid = false;
        return false;
    }

    // Estimate velocity from consecutive simulation states and the actual
    // simulation delta. Advance only by the wall time since the newest sample,
    // bounded to one simulation step and 1/30 second.
    const double predictionAge = std::min({age, step, kMaximumPredictionAge});
    const double dx = static_cast<double>(history.currentPosition.x) - history.previousPosition.x;
    const double dy = static_cast<double>(history.currentPosition.y) - history.previousPosition.y;
    const double segmentDistance = std::hypot(dx, dy);
    double predictedX = dx / step * predictionAge;
    double predictedY = dy / step * predictionAge;
    const double predictedDistance = std::hypot(predictedX, predictedY);

    // Clamp any floating-point overshoot to the last known simulation segment.
    if (segmentDistance <= 0.0 || !std::isfinite(predictedDistance)) return false;
    if (predictedDistance > segmentDistance) {
        const double scale = segmentDistance / predictedDistance;
        predictedX *= scale;
        predictedY *= scale;
    }

    offset = cocos2d::CCPoint(
        static_cast<float>(predictedX),
        static_cast<float>(predictedY)
    );
    return std::isfinite(offset.x) && std::isfinite(offset.y);
}

void setBaseNodePosition(cocos2d::CCNode* node, const cocos2d::CCPoint& position) {
    // Bypass PlayerObject::setPosition: this adjustment is deliberately only
    // the transient Cocos transform used while drawing, not gameplay state.
    node->cocos2d::CCNode::setPosition(position);
}

class TemporaryPlayerTransform {
public:
    explicit TemporaryPlayerTransform(PlayerObject* player) {
        cocos2d::CCPoint offset{};
        if (!player || !predictedOffset(player, offset)) return;

        m_node = static_cast<cocos2d::CCNode*>(player);
        m_originalPosition = m_node->cocos2d::CCNode::getPosition();
        setBaseNodePosition(
            m_node,
            cocos2d::CCPoint(
                m_originalPosition.x + offset.x,
                m_originalPosition.y + offset.y
            )
        );
    }

    TemporaryPlayerTransform(TemporaryPlayerTransform const&) = delete;
    TemporaryPlayerTransform& operator=(TemporaryPlayerTransform const&) = delete;

    ~TemporaryPlayerTransform() {
        if (m_node) setBaseNodePosition(m_node, m_originalPosition);
    }

private:
    cocos2d::CCNode* m_node = nullptr;
    cocos2d::CCPoint m_originalPosition{};
};

} // namespace

void resetHistory() {
    g_history.clear();
}

} // namespace cat::display::extrapolation

#if defined(GEODE_IS_WINDOWS) || defined(GEODE_IS_MACOS) || defined(GEODE_IS_IOS)
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>

class $modify(CatHackExtrapolationPlayer, PlayerObject) {
    void update(float dt) {
        PlayerObject::update(dt);
        cat::display::extrapolation::recordSimulationState(this, dt);
    }
};

// GJBaseGameLayer::visit is an explicit 2.2081 render hook on Windows, macOS,
// and iOS. A single layer-level hook wraps the entire player subtree, avoiding
// a global CCNode::visit detour and restoring both temporary transforms before
// returning to gameplay code.
class $modify(CatHackExtrapolatedLayer, GJBaseGameLayer) {
    void visit() {
        auto* manager = GameManager::get();
        auto* playLayer = manager ? manager->m_playLayer : nullptr;
        if (!playLayer || static_cast<GJBaseGameLayer*>(this) != static_cast<GJBaseGameLayer*>(playLayer) ||
            !cat::display::settings().frameExtrapolation) {
            GJBaseGameLayer::visit();
            return;
        }

        cat::display::extrapolation::TemporaryPlayerTransform firstPlayer(playLayer->m_player1);
        cat::display::extrapolation::TemporaryPlayerTransform secondPlayer(playLayer->m_player2);
        GJBaseGameLayer::visit();
    }
};
#else
// The Android 2.2081 bindings have no GJBaseGameLayer::visit hook address,
// so the UI disables Frame Extrapolation there rather than claiming a render-
// only behavior that cannot be installed through the supported binding API.
#endif
