#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/utils/Keyboard.hpp>
#include <Geode/ui/Notification.hpp>

using namespace geode::prelude;

$execute {
    KeyboardInputEvent()
        .listen(+[](const geode::KeyboardInputData& event) {
            if (event.action != KeyboardInputData::Action::Press) return ListenerResult::Propagate;
            if (event.key != cocos2d::enumKeyCodes::KEY_Q) return ListenerResult::Propagate;

            auto pl = PlayLayer::get();
            if (!pl || !pl->m_effectManager) {
                Notification::create("No active PlayLayer / effect manager", NotificationIcon::Error, 1.5f)->show();
                return ListenerResult::Propagate;
            }

            int itemID = 1;
            int newValue = 16;
            int before = pl->m_effectManager->countForItem(itemID);
            pl->m_effectManager->updateCountForItem(itemID, newValue);
            pl->updateCounters(itemID, newValue);
            int after = pl->m_effectManager->countForItem(itemID);

            log::debug("Item {}: {} -> {}", itemID, before, after);
            Notification::create(fmt::format("Item {} set to {}", itemID, after), NotificationIcon::Success, 1.0f)->show();

            return ListenerResult::Propagate;
        }, -100)
        .leak();
};