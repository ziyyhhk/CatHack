#include "Hacks.hpp"

#include <Geode/Geode.hpp>
#include <Geode/modify/CCScheduler.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>

using namespace geode::prelude;

// Noclip + Safe Mode
class $modify(CatPlayLayer, PlayLayer) {
    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;
        cat::state().cheated = false;
        return true;
    }

    void destroyPlayer(PlayerObject* player, GameObject* object) {
        auto& s = cat::state();
        // the anticheat spike must still kill you, otherwise the level is flagged
        if (s.noclip && object != m_anticheatSpike) {
            s.cheated = true;
            return;
        }
        PlayLayer::destroyPlayer(player, object);
    }

    void levelComplete() {
        auto& s = cat::state();
        bool customPhysicsTps = cat::kPhysicsTpsSupported && s.physicsTpsEnabled && s.physicsTps != 240.0;
        bool cheating = s.cheated || s.noclip || (s.speedEnabled && s.speed != 1.f) || customPhysicsTps;
        // test mode runs are not saved to your stats or submitted
        if (s.safeMode || (s.autoSafeMode && cheating)) {
            m_isTestMode = true;
        }
        PlayLayer::levelComplete();
    }
};

// Speedhack: scale the time step the whole game runs on
class $modify(CatScheduler, CCScheduler) {
    void update(float dt) {
        auto& s = cat::state();
        if (s.speedEnabled) {
            dt *= s.speed;
            if (s.speed != 1.f && PlayLayer::get()) s.cheated = true;
        }
        CCScheduler::update(dt);
    }
};

// No Death Effect
class $modify(CatPlayerObject, PlayerObject) {
    void playDeathEffect() {
        if (cat::state().noDeathEffect) return;
        PlayerObject::playDeathEffect();
    }
};
