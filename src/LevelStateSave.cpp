// LevelStateSave.cpp
#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>
#include <Geode/ui/Popup.hpp>

using namespace geode::prelude;

namespace {
    bool g_shouldRestoreCheckpoint = false;

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
        if (!pl->m_player1 || !pl->m_level) return;
        auto key = fmt::format("checkpoint-{}", pl->m_level->m_levelID.value());

        float x = Mod::get()->getSavedValue<float>(key + "-x", 0.f);
        float y = Mod::get()->getSavedValue<float>(key + "-y", 0.f);

        // Teleport Player 1
        pl->m_player1->setPosition({x, y});
        pl->m_player1->setRotation(Mod::get()->getSavedValue<float>(key + "-rot", 0.f));
        pl->m_player1->m_yVelocity = Mod::get()->getSavedValue<double>(key + "-yvel", 0.0);
        pl->m_player1->m_xVelocity = 0.0; // Reset horizontal momentum
        pl->m_attempts = Mod::get()->getSavedValue<int>(key + "-attempts", pl->m_attempts);

        // Gamemode / Gravity States
        if (Mod::get()->getSavedValue<bool>(key + "-upsideDown", false)) pl->m_player1->flipGravity(true, true);
        if (Mod::get()->getSavedValue<bool>(key + "-ship", false)) pl->m_player1->toggleFlyMode(true, true);
        else if (Mod::get()->getSavedValue<bool>(key + "-ball", false)) pl->m_player1->toggleRollMode(true, true);
        else if (Mod::get()->getSavedValue<bool>(key + "-bird", false)) pl->m_player1->toggleBirdMode(true, true);
        else if (Mod::get()->getSavedValue<bool>(key + "-dart", false)) pl->m_player1->toggleDartMode(true, true);
        else if (Mod::get()->getSavedValue<bool>(key + "-robot", false)) pl->m_player1->toggleRobotMode(true, true);
        else if (Mod::get()->getSavedValue<bool>(key + "-spider", false)) pl->m_player1->toggleSpiderMode(true, true);
        if (Mod::get()->getSavedValue<bool>(key + "-swing", false)) pl->m_player1->toggleSwingMode(true, true);

        // Instantly snap the camera to the player's new position
        pl->updateCamera(0.0f);
    }
}

// 1. Intercept Play Button on LevelInfoLayer
class $modify(MyLevelInfoLayer, LevelInfoLayer) {
    struct Fields {
        bool m_confirmed = false;
    };

    void onPlay(cocos2d::CCObject* sender) {
        if (!m_fields->m_confirmed && m_level && m_level->isPlatformer()) {
            auto key = fmt::format("checkpoint-{}", m_level->m_levelID.value());

            if (Mod::get()->getSavedValue<bool>(key + "-hasSave", false)) {
                createQuickPopup(
                    "Continue?",
                    "Continue at your <cy>last checkpoint</c> in this level?",
                    "No", "Yes",
                    [this, sender](FLAlertLayer*, bool btn2) {
                        m_fields->m_confirmed = true;
                        g_shouldRestoreCheckpoint = btn2;
                        
                        // Proceed to launch level
                        this->onPlay(sender);
                    }
                );
                return;
            }
        }

        LevelInfoLayer::onPlay(sender);
    }
};

// 2. Override Player Position inside PlayLayer::resetLevel
class $modify(LevelStateSave, PlayLayer) {
    CheckpointObject* markCheckpoint() {
        auto cp = PlayLayer::markCheckpoint();
        if (cp) saveCheckpointState(this);
        return cp;
    }

    void resetLevel() {
        // Let GD do its normal reset first
        PlayLayer::resetLevel();

        // If user chose "Yes" on the popup, override player position
        if (g_shouldRestoreCheckpoint) {
            applyCheckpointState(this);
            g_shouldRestoreCheckpoint = false; // Reset flag after applying
        }
    }
};