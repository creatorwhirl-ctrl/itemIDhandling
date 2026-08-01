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

    // Group ID that each of the 16 slots visually represents
    constexpr std::array<int, ItemCount> GroupIDs = {
        5483, 5499, 5503, 5506, 5509, 5511, 5515, 5518,
        5521, 5525, 5527, 5531, 5533, 5536, 5539, 5502
    };

    // Remembers the offset we last applied per group so re-pasting
    // moves groups to the new spot instead of stacking offsets.
    std::array<double, ItemCount> g_lastAppliedOffset{};

    constexpr const char* Alphabet =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";

    char encodeValue(int value) {
        if (value < 0) value = 0;
        if (value > 63) value = 63;
        return Alphabet[value];
    }

    int decodeChar(char c) {
        if (c >= 'A' && c <= 'Z') return c - 'A';
        if (c >= 'a' && c <= 'z') return (c - 'a') + 26;
        if (c >= '0' && c <= '9') return (c - '0') + 52;
        if (c == '+') return 62;
        if (c == '/') return 63;
        return -1;
    }

    // value 0 (A) -> 640 units, value 63 (/) -> 10 units, step of 10
    double offsetForValue(int value) {
        return 640.0 - 10.0 * value;
    }

    std::string formatWithDashes(const std::string& raw) {
        std::string out;
        out.reserve(raw.size() + raw.size() / 4);
        for (size_t i = 0; i < raw.size(); i++) {
            if (i > 0 && i % 4 == 0) out += '-';
            out += raw[i];
        }
        return out;
    }

    std::string stripDashes(const std::string& text) {
        std::string out;
        out.reserve(text.size());
        for (char c : text) {
            if (c != '-') out += c;
        }
        return out;
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

                    std::string raw;
                    raw.reserve(ItemCount);
                    for (int i = 0; i < ItemCount; i++) {
                        int count = pl->m_effectManager->countForItem(SecondItemID + i);
                        raw += encodeValue(count);
                    }

                    auto ok = clipboard::write(formatWithDashes(raw));
                    if (ok) {
                        Notification::create(fmt::format("Copied items {}-{}", SecondItemID, SecondItemID + ItemCount - 1), NotificationIcon::Success, 1.0f)->show();
                    } else {
                        Notification::create("Failed to write clipboard", NotificationIcon::Error, 1.0f)->show();
                    }
                }
                else if (event.key == cocos2d::enumKeyCodes::KEY_V) {
                    if (!pl || !pl->m_effectManager) return ListenerResult::Propagate;

                    std::string clip = stripDashes(clipboard::read());
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
                        // Update the item counter
                        int id = FirstItemID + i;
                        pl->m_effectManager->updateCountForItem(id, parsed[i]);
                        pl->updateCounters(id, parsed[i]);

                        // Move the matching group to reflect the new value
                        double target = offsetForValue(parsed[i]);
                        double delta = target - g_lastAppliedOffset[i];
                        if (delta != 0.0) {
                            pl->moveObjectsSilent(GroupIDs[i], 0.0, delta);
                            g_lastAppliedOffset[i] = target;
                        }
                    }

                    Notification::create(fmt::format("Pasted items {}-{}", FirstItemID, FirstItemID + ItemCount - 1), NotificationIcon::Success, 1.0f)->show();
                }
            }

            return ListenerResult::Propagate;
        }, -100)
        .leak();
};