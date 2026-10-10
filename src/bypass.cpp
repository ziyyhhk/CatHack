// Bypass category. One block per hack, keep the order of the menu.
#include "Hacks.hpp"

#include <Geode/Geode.hpp>
#include <Geode/modify/CCTextInputNode.hpp>
#include <Geode/modify/GameManager.hpp>

using namespace geode::prelude;

// ---------------------------------------------------------------- Text Length
// CCTextInputNode::setMaxLabelLength is inlined in the game, so it can't be
// hooked. The limit is checked when a character is typed instead: raise it
// for the duration of that call and put it back afterwards.
class $modify(BypassTextInput, CCTextInputNode) {
    bool onTextFieldInsertText(CCTextFieldTTF* sender, char const* text, int len, enumKeyCodes key) {
        if (!cat::state().textLength) {
            return CCTextInputNode::onTextFieldInsertText(sender, text, len, key);
        }
        int original = m_maxLabelLength;
        m_maxLabelLength = 9999;
        bool result = CCTextInputNode::onTextFieldInsertText(sender, text, len, key);
        m_maxLabelLength = original;
        return result;
    }
};

// ---------------------------------------------------------------- Unlock Icons
// Local only: the garage thinks everything is unlocked, nothing is sent to the
// servers and your save is not changed.
class $modify(BypassGameManager, GameManager) {
    bool isIconUnlocked(int id, IconType type) {
        if (cat::state().unlockIcons) return true;
        return GameManager::isIconUnlocked(id, type);
    }

    bool isColorUnlocked(int id, UnlockType type) {
        if (cat::state().unlockIcons) return true;
        return GameManager::isColorUnlocked(id, type);
    }
};
