#include <Geode/Geode.hpp>
using namespace geode::prelude;

#include <Geode/modify/PlayLayer.hpp>

class $modify(MyPlayLayer, PlayLayer) {
    // 1. Add the missing `double timestamp` parameter
    void keyDown(cocos2d::enumKeyCodes key, double timestamp) {
        if (key == cocos2d::enumKeyCodes::KEY_U) {
            log::debug("U pressed, m_effectManager is {}", fmt::ptr(this->m_effectManager));

            if (this->m_effectManager) {
                // countForItem returns the current item/item-ID count
                int count = this->m_effectManager->countForItem(1);
                log::debug("Item ID 1 count: {}", count);
            }
        }

        // 2. Pass both arguments to the base function call
        PlayLayer::keyDown(key, timestamp);
    }
};