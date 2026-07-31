#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>

using namespace geode::prelude;

class $modify(MyPlayLayer, PlayLayer) {
	void keyDown(cocos2d::enumKeyCodes key, double dt) {
		if (key == cocos2d::enumKeyCodes::KEY_Q) {
			if (this->m_effectManager) {
				int itemID = 1;
				int newValue = 16;
				int beforeValue = this->m_effectManager->countForItem(itemID);
				this->m_effectManager->updateCountForItem(itemID, newValue);
				this->updateCounters(itemID, newValue); // refreshes any on-screen item label bound to this ID
				int afterValue = this->m_effectManager->countForItem(itemID);
				log::debug("Item {}: {} -> {}", itemID, beforeValue, afterValue);
			}
		}

		PlayLayer::keyDown(key, dt);
	}
};