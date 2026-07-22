#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>

using namespace geode::prelude;

class $modify(MyPlayLayer, PlayLayer) {
    void keyDown(enumKeyCodes key) {
        if (key == enumKeyCodes::KEY_U) {
            if (this->m_effectManager) {
                this->m_effectManager->setItemCount(1, 13, false, 0);
                Notification::create("Set Item ID 1 to 13!", NotificationIcon::Success)->show();
            }
        }

        PlayLayer::keyDown(key);
    }
};