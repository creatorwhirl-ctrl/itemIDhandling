// LevelStateSave.cpp
#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/ui/Popup.hpp>

using namespace geode::prelude;

namespace {
    void saveCheckpointState(PlayLayer* pl) {
        if (!pl->m_isPlatformer || !pl->m_level || !pl->m_player1) return;
        auto key = fmt::format("checkpoint-{}", pl->m_level->m_levelID.value());

        Mod::get()->setSavedValue<bool>(key + "-hasSave", true);
        Mod::get()->setSavedValue<float>(key + "-x", pl->m_player1->getPositionX());
        Mod::get()->setSavedValue<float>(key + "-y", pl->m_player1->getPositionY());
        Mod::get()->setSavedValue<float>(key + "-rot", pl->m_player1->getRotation());
        Mod::get()->setSavedValue<double>(key + "-yvel", pl->m_player1->m_yVelocity);
        Mod::get()->setSavedValue<bool>(key + "-upsideDown", pl->m_player1->m_isUpsideDown);
        Mod::get()->setSavedValue<bool>(key + "-ship", pl->m_player1->m_isShip);
        Mod::get()->setSavedValue<bool>(key + "-ball", pl->m_player1->m_isBall);
        Mod::get()->setSavedValue<bool>(key + "-bird", pl->m_player1->m_isBird);
        Mod::get()->setSavedValue<bool>(key + "-dart", pl->m_player1->m_isDart);
        Mod::get()->setSavedValue<bool>(key + "-robot", pl->m_player1->m_isRobot);
        Mod::get()->setSavedValue<bool>(key + "-spider", pl->m_player1->m_isSpider);
        Mod::get()->setSavedValue<bool>(key + "-swing", pl->m_player1->m_isSwing);
        Mod::get()->setSavedValue<int>(key + "-attempts", pl->m_attempts);
    }

    void applyCheckpointState(PlayLayer* pl) {
        auto key = fmt::format("checkpoint-{}", pl->m_level->m_levelID.value());

        float x = Mod::get()->getSavedValue<float>(key + "-x", 0.f);
        float y = Mod::get()->getSavedValue<float>(key + "-y", 0.f);
        pl->m_player1->setPosition({x, y});
        pl->m_player1->setRotation(Mod::get()->getSavedValue<float>(key + "-rot", 0.f));
        pl->m_player1->m_yVelocity = Mod::get()->getSavedValue<double>(key + "-yvel", 0.0);
        pl->m_attempts = Mod::get()->getSavedValue<int>(key + "-attempts", pl->m_attempts);

        if (Mod::get()->getSavedValue<bool>(key + "-upsideDown", false)) pl->m_player1->flipGravity(true, true);
        if (Mod::get()->getSavedValue<bool>(key + "-ship", false)) pl->m_player1->toggleFlyMode(true, true);
        else if (Mod::get()->getSavedValue<bool>(key + "-ball", false)) pl->m_player1->toggleRollMode(true, true);
        else if (Mod::get()->getSavedValue<bool>(key + "-bird", false)) pl->m_player1->toggleBirdMode(true, true);
        else if (Mod::get()->getSavedValue<bool>(key + "-dart", false)) pl->m_player1->toggleDartMode(true, true);
        else if (Mod::get()->getSavedValue<bool>(key + "-robot", false)) pl->m_player1->toggleRobotMode(true, true);
        else if (Mod::get()->getSavedValue<bool>(key + "-spider", false)) pl->m_player1->toggleSpiderMode(true, true);
        if (Mod::get()->getSavedValue<bool>(key + "-swing", false)) pl->m_player1->toggleSwingMode(true, true);
    }
}

class $modify(LevelStateSave, PlayLayer) {
    struct Fields {
        bool m_decided = false;
        bool m_wantsResume = false;
        bool m_waitingForPopup = false;
    };

    CheckpointObject* markCheckpoint() {
        auto cp = PlayLayer::markCheckpoint();
        if (cp) saveCheckpointState(this);
        return cp;
    }

    void setupHasCompleted() {
        if (!m_fields->m_decided && m_isPlatformer && m_level) {
            auto key = fmt::format("checkpoint-{}", m_level->m_levelID.value());

            if (Mod::get()->getSavedValue<bool>(key + "-hasSave", false)) {
                if (!m_fields->m_waitingForPopup) {
                    m_fields->m_waitingForPopup = true;
                    m_loadingProgress = 0.99f;

                    createQuickPopup(
                        "Continue?",
                        "Continue at your <cy>last checkpoint</c> in this level?",
                        "No", "Yes",
                        [this](FLAlertLayer*, bool btn2) {
                            m_fields->m_decided = true;
                            m_fields->m_wantsResume = btn2;
                            // Don't call setupHasCompleted() ourselves - just let
                            // the engine's own loading loop re-poll this on the
                            // next frame now that m_decided is true.
                            m_loadingProgress = 1.0f;
                        }
                    );
                }
                return; // still waiting on the popup, don't fall through yet
            }
            m_fields->m_decided = true;
        }

        PlayLayer::setupHasCompleted();

        if (m_fields->m_wantsResume) {
            applyCheckpointState(this);
            m_fields->m_wantsResume = false;
        }
    }
};