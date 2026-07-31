#include <Geode/Geode.hpp>
#include <Geode/utils/Keyboard.hpp>
#include <Geode/ui/Notification.hpp>
#include <array>

using namespace geode::prelude;

namespace {
    constexpr int kFirstItemID = 1;
    constexpr int kItemCount = 16;
    std::array<int, kItemCount> g_itemClipboard{};
    bool g_hasCopied = false;
}

$execute {
    KeyboardInputEvent()
        .listen(+[](const geode::KeyboardInputData& event) {
            if (event.action != KeyboardInputData::Action::Press) return ListenerResult::Propagate;

            auto pl = PlayLayer::get();

            // Q: set item 1 to 16 (your existing test key)
            if (event.key == cocos2d::enumKeyCodes::KEY_Q) {
                if (!pl || !pl->m_effectManager) {
                    Notification::create("No active PlayLayer / effect manager", NotificationIcon::Error, 1.5f)->show();
                    return ListenerResult::Propagate;
                }
                pl->m_effectManager->updateCountForItem(1, 16);
                pl->updateCounters(1, 16);
                Notification::create("Item 1 set to 16", NotificationIcon::Success, 1.0f)->show();
                return ListenerResult::Propagate;
            }

            // Ctrl+C / Ctrl+V: copy-paste item IDs 1-16
            if (event.modifiers & KeyboardModifier::Control) {
                if (event.key == cocos2d::enumKeyCodes::KEY_C) {
                    if (!pl || !pl->m_effectManager) return ListenerResult::Propagate;
                    for (int i = 0; i < kItemCount; i++) {
                        g_itemClipboard[i] = pl->m_effectManager->countForItem(kFirstItemID + i);
                    }
                    g_hasCopied = true;
                    Notification::create(fmt::format("Copied items {}-{}", kFirstItemID, kFirstItemID + kItemCount - 1), NotificationIcon::Success, 1.0f)->show();
                }
                else if (event.key == cocos2d::enumKeyCodes::KEY_V) {
                    if (!pl || !pl->m_effectManager) return ListenerResult::Propagate;
                    if (!g_hasCopied) {
                        Notification::create("Nothing copied yet", NotificationIcon::Error, 1.0f)->show();
                        return ListenerResult::Propagate;
                    }
                    for (int i = 0; i < kItemCount; i++) {
                        int id = kFirstItemID + i;
                        pl->m_effectManager->updateCountForItem(id, g_itemClipboard[i]);
                        pl->updateCounters(id, g_itemClipboard[i]);
                    }
                    Notification::create(fmt::format("Pasted items {}-{}", kFirstItemID, kFirstItemID + kItemCount - 1), NotificationIcon::Success, 1.0f)->show();
                }
            }

            return ListenerResult::Propagate;
        }, -100)
        .leak();
};