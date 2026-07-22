#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>

using namespace geode::prelude;

class $modify(MyPlayLayer, PlayLayer) {

	void keyDown(cocos2d::enumKeyCodes key) {
		if (key == cocos2d::enumKeyCodes::KEY_U) {

			log::debug("U pressed. m_effectManager = {}", fmt::ptr(this->m_effectManager));

			if (this->m_effectManager) {
				int itemID = 1;
				int newValue = 10;

				int beforeValue = this->m_effectManager->countForItem(itemID);
				this->m_effectManager->updateCountForItem(itemID, newValue);
				int afterValue = this->m_effectManager->countForItem(itemID);

				log::debug("Item {}: {} -> {}", itemID, beforeValue, afterValue);
			}
		}

		PlayLayer::keyDown(key);
	}
};