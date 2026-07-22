#include <Geode/Geode.hpp>
using namespace geode::prelude;

#include <Geode/modify/PlayLayer.hpp>

class $modify(MyPlayLayer, PlayLayer) {
    void keyDown(cocos2d::enumKeyCodes key, double timestamp) {
        if (key == cocos2d::enumKeyCodes::KEY_U) {

            // 1. Show popup window
            FLAlertLayer::create(
                "Item Modified!",           // Title
                "Item ID 1 count set to 5", // Message body
                "OK"                        // Button text
            )->show();

            // 2. Change the Item ID count using updateItemValue
            if (this->m_effectManager) {
                // updateItemValue(itemID, count, isDelta)
                // isDelta = false sets the absolute value directly to 5
                this->m_effectManager->updateItemValue(1, 5, false);
                
                log::info("Updated Item ID 1 to 5");
            }
        }

        // Call base function to retain original keyboard functionality
        PlayLayer::keyDown(key, timestamp);
    }
};