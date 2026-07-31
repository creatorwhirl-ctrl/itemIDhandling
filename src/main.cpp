#include <Geode/Geode.hpp>
#include <Geode/utils/Keyboard.hpp>
#include <Geode/utils/general.hpp>
#include <Geode/ui/Notification.hpp>
#include <array>
#include <string>

using namespace geode::prelude;

namespace {
    constexpr int FirstItemID = 1225;
    constexpr int SecondItemID = 1200;
    constexpr int ItemCount = 16;

    constexpr const char* Alphabet =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";

    char encodeValue(int value) {
        // Clamp to the encodable range so out-of-range item counts don't produce garbage
        if (value < 0) value = 0;
        if (value > 63) value = 63;
        return Alphabet[value];
    }

    // Returns -1 if the char isn't a valid alphabet character
    int decodeChar(char c) {
        if (c >= 'A' && c <= 'Z') return c - 'A';
        if (c >= 'a' && c <= 'z') return (c - 'a') + 26;
        if (c >= '0' && c <= '9') return (c - '0') + 52;
        if (c == '+') return 62;
        if (c == '/') return 63;
        return -1;
    }
}

$execute {
    KeyboardInputEvent()
        .listen(+[](const geode::KeyboardInputData& event) {
            if (event.action != KeyboardInputData::Action::Press) return ListenerResult::Propagate;

            auto pl = PlayLayer::get();

            if (event.modifiers & KeyboardModifier::Control) {
                if (event.key == cocos2d::enumKeyCodes::KEY_C) {
                    if (!pl || !pl->m_effectManager) return ListenerResult::Propagate;

                    std::string encoded;
                    encoded.reserve(ItemCount);
                    for (int i = 0; i < ItemCount; i++) {
                        int count = pl->m_effectManager->countForItem(SecondItemID + i);
                        encoded += encodeValue(count);
                    }

                    auto ok = clipboard::write(encoded);
                    if (ok) {
                        Notification::create(fmt::format("Copied items {}-{}", SecondItemID, SecondItemID + ItemCount - 1), NotificationIcon::Success, 1.0f)->show();
                    } else {
                        Notification::create("Failed to write clipboard", NotificationIcon::Error, 1.0f)->show();
                    }
                }
                else if (event.key == cocos2d::enumKeyCodes::KEY_V) {
                    if (!pl || !pl->m_effectManager) return ListenerResult::Propagate;

                    std::string clip = clipboard::read();
                    if (clip.size() != ItemCount) {
                        Notification::create("Clipboard text isn't valid item data", NotificationIcon::Error, 1.0f)->show();
                        return ListenerResult::Propagate;
                    }

                    std::array<int, ItemCount> parsed{};
                    for (int i = 0; i < ItemCount; i++) {
                        int value = decodeChar(clip[i]);
                        if (value < 0) {
                            Notification::create("Clipboard text isn't valid item data", NotificationIcon::Error, 1.0f)->show();
                            return ListenerResult::Propagate;
                        }
                        parsed[i] = value;
                    }

                    for (int i = 0; i < ItemCount; i++) {
                        int id = FirstItemID + i;
                        pl->m_effectManager->updateCountForItem(id, parsed[i]);
                        pl->updateCounters(id, parsed[i]);
                    }

                    Notification::create(fmt::format("Pasted items {}-{}", FirstItemID, FirstItemID + ItemCount - 1), NotificationIcon::Success, 1.0f)->show();
                }
            }

            return ListenerResult::Propagate;
        }, -100)
        .leak();
};