#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>
#include <Geode/ui/Popup.hpp>

using namespace geode::prelude;

namespace {
    bool g_shouldRestoreCheckpoint = false;

    std::string getSaveKey(int levelID) {
        return fmt::format("checkpoint-data-{}", levelID);
    }

    // Pull everything relevant off the real PlayerCheckpoint the game just made.
    void saveCheckpointData(PlayLayer* pl, CheckpointObject* cp) {
        if (!pl || !pl->m_level || !cp || !cp->m_player1Checkpoint) return;

        auto pc = cp->m_player1Checkpoint;
        auto key = getSaveKey(pl->m_level->m_levelID.value());

        Mod::get()->setSavedValue<bool>(key + "-exists", true);

        Mod::get()->setSavedValue<float>(key + "-x", pc->m_position.x);
        Mod::get()->setSavedValue<float>(key + "-y", pc->m_position.y);
        Mod::get()->setSavedValue<double>(key + "-yvel", pc->m_yVelocity);
        Mod::get()->setSavedValue<float>(key + "-rot", pc->m_rotation);
        Mod::get()->setSavedValue<float>(key + "-gravity", pc->m_gravityMod);

        Mod::get()->setSavedValue<bool>(key + "-upsidedown", pc->m_isUpsideDown);
        Mod::get()->setSavedValue<bool>(key + "-mini", pc->m_isMini);
        Mod::get()->setSavedValue<bool>(key + "-ship", pc->m_isShip);
        Mod::get()->setSavedValue<bool>(key + "-ball", pc->m_isBall);
        Mod::get()->setSavedValue<bool>(key + "-bird", pc->m_isBird);
        Mod::get()->setSavedValue<bool>(key + "-dart", pc->m_isDart);
        Mod::get()->setSavedValue<bool>(key + "-robot", pc->m_isRobot);
        Mod::get()->setSavedValue<bool>(key + "-spider", pc->m_isSpider);
        Mod::get()->setSavedValue<bool>(key + "-swing", pc->m_isSwing);
        Mod::get()->setSavedValue<bool>(key + "-onground", pc->m_isOnGround);

        Mod::get()->setSavedValue<int>(key + "-attempts", pl->m_attempts);
    }

    // Build a *real* checkpoint (valid pointers, valid PlayerCheckpoint), then
    // overwrite it with saved values instead of trying to construct one from nothing.
    void loadCheckpointData(PlayLayer* pl) {
        if (!pl || !pl->m_level) return;

        auto key = getSaveKey(pl->m_level->m_levelID.value());
        if (!Mod::get()->getSavedValue<bool>(key + "-exists", false)) return;

        auto cp = pl->markCheckpoint();
        if (!cp || !cp->m_player1Checkpoint) return;

        auto pc = cp->m_player1Checkpoint;

        pc->m_position.x = Mod::get()->getSavedValue<float>(key + "-x", pc->m_position.x);
        pc->m_position.y = Mod::get()->getSavedValue<float>(key + "-y", pc->m_position.y);
        pc->m_yVelocity  = Mod::get()->getSavedValue<double>(key + "-yvel", pc->m_yVelocity);
        pc->m_rotation   = Mod::get()->getSavedValue<float>(key + "-rot", pc->m_rotation);
        pc->m_gravityMod = Mod::get()->getSavedValue<float>(key + "-gravity", pc->m_gravityMod);

        pc->m_isUpsideDown = Mod::get()->getSavedValue<bool>(key + "-upsidedown", false);
        pc->m_isMini       = Mod::get()->getSavedValue<bool>(key + "-mini", false);
        pc->m_isShip       = Mod::get()->getSavedValue<bool>(key + "-ship", false);
        pc->m_isBall       = Mod::get()->getSavedValue<bool>(key + "-ball", false);
        pc->m_isBird       = Mod::get()->getSavedValue<bool>(key + "-bird", false);
        pc->m_isDart       = Mod::get()->getSavedValue<bool>(key + "-dart", false);
        pc->m_isRobot      = Mod::get()->getSavedValue<bool>(key + "-robot", false);
        pc->m_isSpider     = Mod::get()->getSavedValue<bool>(key + "-spider", false);
        pc->m_isSwing      = Mod::get()->getSavedValue<bool>(key + "-swing", false);
        pc->m_isOnGround   = Mod::get()->getSavedValue<bool>(key + "-onground", true);

        pl->m_attempts = Mod::get()->getSavedValue<int>(key + "-attempts", pl->m_attempts);

        pl->loadFromCheckpoint(cp);
        pl->updateCamera(0.0f);
    }
}

// LevelInfoLayer Popup Intercept
class $modify(MyLevelInfoLayer, LevelInfoLayer) {
    struct Fields {
        bool m_confirmed = false;
    };

    void onPlay(cocos2d::CCObject* sender) {
        if (!m_fields->m_confirmed && m_level && m_level->isPlatformer()) {
            auto key = getSaveKey(m_level->m_levelID.value());

            if (Mod::get()->getSavedValue<bool>(key + "-exists", false)) {
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

// PlayLayer Hook
class $modify(LevelStateSave, PlayLayer) {
    struct Fields {
        bool m_isRestoring = false;
    };

    CheckpointObject* markCheckpoint() {
        auto cp = PlayLayer::markCheckpoint();
        if (cp && this->m_isPlatformer && !m_fields->m_isRestoring) {
            saveCheckpointData(this, cp);
        }
        return cp;
    }

    void resetLevel() {
        PlayLayer::resetLevel();

        if (g_shouldRestoreCheckpoint) {
            g_shouldRestoreCheckpoint = false;
            m_fields->m_isRestoring = true;
            loadCheckpointData(this);
            m_fields->m_isRestoring = false;
        }
    }
};