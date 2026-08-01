#include <Geode/Geode.hpp>
#include <Geode/utils/Keyboard.hpp>
#include <Geode/utils/general.hpp>
#include <Geode/ui/Notification.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <array>
#include <string>

using namespace geode::prelude;

namespace {
    constexpr int FirstItemID = 1225;
    constexpr int SecondItemID = 1200;
    constexpr int ItemCount = 16;

    constexpr std::array<int, ItemCount> GroupIDs = {
        5483, 5499, 5503, 5506, 5509, 5511, 5515, 5518,
        5521, 5525, 5527, 5531, 5533, 5536, 5539, 5502
    };

    std::array<double, ItemCount> g_baseGroupY{};
    std::array<bool, ItemCount> g_baseCaptured{};

    bool g_smallStep = false;

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

    void captureBaselines(PlayLayer* pl) {
        for (int i = 0; i < ItemCount; i++) {
            auto group = pl->getGroup(GroupIDs[i]);
            if (group && group->count() > 0) {
                auto obj = static_cast<GameObject*>(group->objectAtIndex(0));
                g_baseGroupY[i] = obj->m_positionY;
                g_baseCaptured[i] = true;
            } else {
                g_baseCaptured[i] = false;
            }
        }
    }
}

class $modify(GroupOffsetTracker, PlayLayer) {
    struct Fields {
        int captureAttempts = 0;
    };

    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;
        m_fields->captureAttempts = 0;
        this->schedule(schedule_selector(GroupOffsetTracker::tryCaptureBaselines));
        return true;
    }

    void tryCaptureBaselines(float) {
        captureBaselines(this);
        m_fields->captureAttempts++;

        bool allCaptured = true;
        for (bool captured : g_baseCaptured) {
            if (!captured) { allCaptured = false; break; }
        }

        if (allCaptured || m_fields->captureAttempts >= 30) {
            this->unschedule(schedule_selector(GroupOffsetTracker::tryCaptureBaselines));
        }
    }
};

$execute {
    KeyboardInputEvent()
        .listen(+[](const geode::KeyboardInputData& event) {
            if (event.action != KeyboardInputData::Action::Press) return ListenerResult::Propagate;

            auto pl = PlayLayer::get();

            if (event.modifiers & KeyboardModifier::Control) {
                if (event.modifiers & KeyboardModifier::Alt && event.key == cocos2d::enumKeyCodes::KEY_S) {
                    g_smallStep = !g_smallStep;
                    Notification::create(fmt::format("Small step: {}", g_smallStep ? "ON" : "OFF"), NotificationIcon::Success, 1.0f)->show();
                    return ListenerResult::Propagate;
                }

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

                    constexpr double blockUnit = 30.0;

                    for (int i = 0; i < ItemCount; i++) {
                        int id = FirstItemID + i;
                        pl->m_effectManager->updateCountForItem(id, parsed[i]);
                        pl->updateCounters(id, parsed[i]);

                        if (!g_baseCaptured[i]) continue;

                        auto group = pl->getGroup(GroupIDs[i]);
                        if (!group || group->count() == 0) continue;

                        auto anchor = static_cast<GameObject*>(group->objectAtIndex(0));
                        double currentY = anchor->m_positionY;

                        int blocksFromTop = 75 - parsed[i];
                        double targetY = g_baseGroupY[i] - (blocksFromTop * blockUnit);

                        double delta = targetY - currentY;
                        if (delta != 0.0) {
                            pl->moveObjectsSilent(GroupIDs[i], 0.0, delta);
                        }
                    }

                    Notification::create(fmt::format("Pasted items {}-{}", FirstItemID, FirstItemID + ItemCount - 1), NotificationIcon::Success, 1.0f)->show();
                }
            }

            return ListenerResult::Propagate;
        }, -100)
        .leak();
};