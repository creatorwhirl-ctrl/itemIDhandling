#include <Geode/Geode.hpp>
using namespace geode::prelude;

#include <Geode/modify/PlayLayer.hpp>
class $modify(MyPlayLayer, PlayLayer) {
	void keyDown(cocos2d::enumKeyCodes key) {
		if (key == cocos2d::enumKeyCodes::KEY_U) {
			log::debug("U pressed, m_effectManager is {}", fmt::ptr(this->m_effectManager));

			if (this->m_effectManager) {
				// this->m_effectManager->countForItem(1);
			}
		}

		PlayLayer::keyDown(key);
	}
};