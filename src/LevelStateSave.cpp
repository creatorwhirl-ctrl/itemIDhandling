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

        // Store physical positioning
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

        // Create genuine native CheckpointObject
        auto cp = CheckpointObject::create();
        if (!cp) return;

        // Register checkpoint in PlayLayer's native array
        if (pl->m_checkpointArray) {
            pl->m_checkpointArray->addObject(cp);
        }

        // Run native GD checkpoint restoration pass
        pl->loadFromCheckpoint(cp);

        // Position player cleanly
        float x = Mod::get()->getSavedValue<float>(key + "-x", 0.f);
        float y = Mod::get()->getSavedValue<float>(key + "-y", 0.f);

        if (pl->m_player1) {
            pl->m_player1->setPosition({x, y});
            pl->m_player1->m_yVelocity = Mod::get()->getSavedValue<double>(key + "-yvel", 0.0);
        }

        pl->updateCamera(0.0f);
    }
}

// 1. LevelInfoLayer Popup Intercept
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

// 2. PlayLayer Hook
class $modify(LevelStateSave, PlayLayer) {
    CheckpointObject* markCheckpoint() {
        auto cp = PlayLayer::markCheckpoint();
        if (cp && this->m_isPlatformer) {
            saveCheckpointData(this);
        }
        return cp;
    }

    void resetLevel() {
        PlayLayer::resetLevel();

        if (g_shouldRestoreCheckpoint) {
            g_shouldRestoreCheckpoint = false;
            loadCheckpointData(this);
        }
    }
};