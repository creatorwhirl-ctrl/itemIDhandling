#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>
#include <Geode/ui/Popup.hpp>

using namespace geode::prelude;

namespace {
    bool g_shouldRestoreCheckpoint = false;

    // Structure holding complete snapshot data
    struct SaveData {
        float x = 0.f;
        float y = 0.f;
        float rot = 0.f;
        double yVel = 0.0;
        bool upsideDown = false;
        bool ship = false;
        bool ball = false;
        bool bird = false;
        bool dart = false;
        bool robot = false;
        bool spider = false;
        bool swing = false;
        int attempts = 0;
    };

    void saveCheckpointState(PlayLayer* pl) {
        if (!pl->m_isPlatformer || !pl->m_level || !pl->m_player1) return;
        auto key = fmt::format("checkpoint-{}", pl->m_level->m_levelID.value());

        // 1. Save Player Transform & Modes
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

    void restoreNativeState(PlayLayer* pl) {
        if (!pl->m_player1 || !pl->m_level) return;
        auto key = fmt::format("checkpoint-{}", pl->m_level->m_levelID.value());

        // Create a native CheckpointObject
        auto cp = CheckpointObject::create();
        if (!cp) return;

        float x = Mod::get()->getSavedValue<float>(key + "-x", 0.f);
        float y = Mod::get()->getSavedValue<float>(key + "-y", 0.f);

        // Populate native checkpoint fields
        cp->m_physicalPosition = {x, y};
        cp->m_player1IsUpsideDown = Mod::get()->getSavedValue<bool>(key + "-upsideDown", false);

        // Push to PlayLayer's official checkpoint array
        if (pl->m_checkpointArray) {
            pl->m_checkpointArray->addObject(cp);
        }

        // Force GD to load natively from this checkpoint
        pl->loadFromCheckpoint(cp);

        // Ensure player position and momentum match strictly
        pl->m_player1->setPosition({x, y});
        pl->m_player1->m_yVelocity = Mod::get()->getSavedValue<double>(key + "-yvel", 0.0);
        pl->m_player1->m_xVelocity = 0.0;
        
        // Restore gamemodes safely
        if (Mod::get()->getSavedValue<bool>(key + "-ship", false)) pl->m_player1->toggleFlyMode(true, true);
        else if (Mod::get()->getSavedValue<bool>(key + "-ball", false)) pl->m_player1->toggleRollMode(true, true);
        else if (Mod::get()->getSavedValue<bool>(key + "-bird", false)) pl->m_player1->toggleBirdMode(true, true);
        else if (Mod::get()->getSavedValue<bool>(key + "-dart", false)) pl->m_player1->toggleDartMode(true, true);
        else if (Mod::get()->getSavedValue<bool>(key + "-robot", false)) pl->m_player1->toggleRobotMode(true, true);
        else if (Mod::get()->getSavedValue<bool>(key + "-spider", false)) pl->m_player1->toggleSpiderMode(true, true);
        if (Mod::get()->getSavedValue<bool>(key + "-swing", false)) pl->m_player1->toggleSwingMode(true, true);

        // Update camera position immediately
        pl->updateCamera(0.0f);
    }
}

// 1. Popup on LevelInfoLayer before PlayLayer loads
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
                        this->onPlay(sender);
                    }
                );
                return;
            }
        }

        LevelInfoLayer::onPlay(sender);
    }
};

// 2. Load Native Checkpoint during level initialization
class $modify(LevelStateSave, PlayLayer) {
    CheckpointObject* markCheckpoint() {
        auto cp = PlayLayer::markCheckpoint();
        if (cp) {
            saveCheckpointState(this);
        }
        return cp;
    }

    void resetLevel() {
        PlayLayer::resetLevel();

        if (g_shouldRestoreCheckpoint) {
            g_shouldRestoreCheckpoint = false; // Consume flag
            restoreNativeState(this);
        }
    }
};