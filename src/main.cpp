#include <Geode/Geode.hpp>
using namespace geode::prelude;

#include <Geode/modify/PlayLayer.hpp>

class $modify(MyPlayLayer, PlayLayer) {
    void keyDown(cocos2d::enumKeyCodes key, double timestamp) {
        if (key == cocos2d::enumKeyCodes::KEY_U) {

            // 1. Create and show a popup window using FLAlertLayer
            FLAlertLayer::create(
                "Item Modified!",           // Title
                "Item ID 1 count set to 5", // Message body
                "OK"                        // Button text
            )->show();

            // 2. Change the Item ID count in the current level
            if (this->m_effectManager) {
                // updateItem modifies/sets the item count for the given ID
                this->m_effectManager->updateItem(1, 5, false);
                
                log::info("Updated Item ID 1 to 5");
            }
        }

        // Call base GD function to keep regular key controls working
        PlayLayer::keyDown(key, timestamp);
    }
};