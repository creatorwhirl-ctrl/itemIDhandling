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

    void saveCheckpointData(PlayLayer* pl) {
        if (!pl || !pl->m_level || !pl->m_player1) return;
        
        auto key = getSaveKey(pl->m_level->m_levelID.value());

        Mod::get()->setSavedValue<bool>(key + "-exists", true);
        Mod::get()->setSavedValue<float>(key + "-x", pl->m_player1->getPositionX());
        Mod::get()->setSavedValue<float>(key + "-y", pl->m_player1->getPositionY());
        Mod::get()->setSavedValue<double>(key + "-yvel", pl->m_player1->m_yVelocity);
        Mod::get()->setSavedValue<int>(key + "-attempts", pl->m_attempts);
    }

    void loadCheckpointData(PlayLayer* pl) {
        if (!pl || !pl->m_level) return;
        
        auto key = getSaveKey(pl->m_level->m_levelID.value());
        if (!Mod::get()->getSavedValue<bool>(key + "-exists", false)) return;

        // 1. Create a native CheckpointObject using GD's engine function
        auto cp = pl->markCheckpoint();
        if (!cp) return;

        // 2. Read saved positioning details
        float x = Mod::get()->getSavedValue<float>(key + "-x", 0.f);
        float y = Mod::get()->getSavedValue<float>(key + "-y", 0.f);
        double yVel = Mod::get()->getSavedValue<double>(key + "-yvel", 0.0);

        // 3. Trigger GD's internal checkpoint loading logic
        pl->loadFromCheckpoint(cp);

        // 4. Reposition player cleanly
        if (pl->m_player1) {
            pl->m_player1->setPosition({x, y});
            pl->m_player1->m_yVelocity = yVel;
        }

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
        // Avoid recursive saving while restoring checkpoint
        if (cp && this->m_isPlatformer && !m_fields->m_isRestoring) {
            saveCheckpointData(this);
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