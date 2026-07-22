#include <Geode/Geode.hpp>
using namespace geode::prelude;

#include <Geode/modify/PlayLayer.hpp>

class $modify(MyPlayLayer, PlayLayer) {
    void keyDown(cocos2d::enumKeyCodes key, double timestamp) {
        if (key == cocos2d::enumKeyCodes::KEY_U) {

            // 1. Show alert popup on screen
            FLAlertLayer::create(
                "Item Modified!",           // Title
                "Item ID 1 count set to 5", // Message body
                "OK"                        // Button text
            )->show();

            // 2. Set item count directly in the effect manager map
            if (this->m_effectManager) {
                // Set Item ID 1 count to 5 in the m_itemValues map
                this->m_effectManager->m_itemValues[1] = 5;

                // Read back using countForItem (confirmed in bindings)
                int currentCount = this->m_effectManager->countForItem(1);
                log::info("Successfully updated Item ID 1 to: {}", currentCount);
            }
        }

        // Call regular base key event so controls keep working
        PlayLayer::keyDown(key, timestamp);
    }
};