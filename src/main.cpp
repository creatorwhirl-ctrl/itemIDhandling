#include <Geode/Geode.hpp>
#include <Geode/utils/Keyboard.hpp>
#include <Geode/utils/general.hpp>
#include <Geode/ui/Notification.hpp>
#include <array>
#include <sstream>

using namespace geode::prelude;

namespace {
    constexpr int FirstItemID = 1225;
    constexpr int SecondItemID = 1200;
    constexpr int ItemCount = 16;
}

$execute {
    KeyboardInputEvent()
        .listen(+[](const geode::KeyboardInputData& event) {
            if (event.action != KeyboardInputData::Action::Press) return ListenerResult::Propagate;

            auto pl = PlayLayer::get();

            if (event.modifiers & KeyboardModifier::Control) {
                if (event.key == cocos2d::enumKeyCodes::KEY_C) {
                    if (!pl || !pl->m_effectManager) return ListenerResult::Propagate;

                    std::ostringstream oss;
                    for (int i = 0; i < ItemCount; i++) {
                        int id = SecondItemID + i;
                        int count = pl->m_effectManager->countForItem(id);
                        if (i > 0) oss << ",";
                        oss << id << ":" << count;
                    }

                    auto ok = clipboard::write(oss.str());
                    if (ok) {
                        Notification::create(fmt::format("Copied items {}-{}", SecondItemID, SecondItemID + ItemCount - 1), NotificationIcon::Success, 1.0f)->show();
                    } else {
                        Notification::create("Failed to write clipboard", NotificationIcon::Error, 1.0f)->show();
                    }
                }
                else if (event.key == cocos2d::enumKeyCodes::KEY_V) {
                    if (!pl || !pl->m_effectManager) return ListenerResult::Propagate;

                    std::string clip = clipboard::read();
                    if (clip.empty()) {
                        Notification::create("Clipboard is empty", NotificationIcon::Error, 1.0f)->show();
                        return ListenerResult::Propagate;
                    }

                    // Parse "id:value,id:value,..."
                    std::array<int, ItemCount> parsed{};
                    bool valid = true;
                    int found = 0;

                    std::stringstream ss(clip);
                    std::string token;
                    while (std::getline(ss, token, ',')) {
                        auto colon = token.find(':');
                        if (colon == std::string::npos) { valid = false; break; }

                        try {
                            int id = std::stoi(token.substr(0, colon));
                            int value = std::stoi(token.substr(colon + 1));
                            int idx = id - SecondItemID;
                            if (idx < 0 || idx >= ItemCount) { valid = false; break; }
                            parsed[idx] = value;
                            found++;
                        } catch (...) {
                            valid = false;
                            break;
                        }
                    }

                    if (!valid || found != ItemCount) {
                        Notification::create("Clipboard text isn't valid item data", NotificationIcon::Error, 1.0f)->show();
                        return ListenerResult::Propagate;
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