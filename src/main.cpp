#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>

using namespace geode::prelude;

class $modify(MyPlayLayer, PlayLayer) {
    void keyDown(cocos2d::enumKeyCodes key) {
        // Check if player pressed 'U' while in a level
        if (key == cocos2d::enumKeyCodes::KEY_U) {
            if (this->m_effectManager) {
                // Set Item ID 1 to value 13 immediately
                this->m_effectManager->setItemCount(1, 13, false, 0);
                
                // Show a quick notification on screen
                Notification::create("Set Item ID 1 to 13!", NotificationIcon::Success)->show();
            }
        }

        // Always call the original keyDown so normal controls still work
        PlayLayer::keyDown(key);
    }
};