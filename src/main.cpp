#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>

using namespace geode::prelude;

class $modify(MyPlayLayer, PlayLayer) {

	void keyDown(cocos2d::enumKeyCodes key, double dt) {

		// DIAGNOSTIC: logs every single key press so we can confirm the
		// hook is firing at all, regardless of which key is pressed.
		log::debug("keyDown fired, key = {}", (int)key);

		if (key == cocos2d::enumKeyCodes::KEY_J) {

			// Sanity check: confirms the hook fired and the effect manager
			// is valid before we try to touch it. Check this first in the
			// in-game console (Geode Settings -> Console) if item ID 1
			// still doesn't change after pressing J.
			log::debug("J pressed. m_effectManager = {}", fmt::ptr(this->m_effectManager));

			if (this->m_effectManager) {
				int itemID = 1;
				int newValue = 10;

				int beforeValue = this->m_effectManager->countForItem(itemID);
				this->m_effectManager->updateCountForItem(itemID, newValue);
				int afterValue = this->m_effectManager->countForItem(itemID);

				log::debug("Item {}: {} -> {}", itemID, beforeValue, afterValue);
			}
		}

		// Always forward to the original so other keybinds keep working.
		PlayLayer::keyDown(key, dt);
	}
};