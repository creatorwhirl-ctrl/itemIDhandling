#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>
#include <Geode/ui/Popup.hpp>

using namespace geode::prelude;

namespace {
    bool g_shouldRestoreCheckpoint = false;

    // Save key helper
    std::string getSaveKey(int levelID) {
        return fmt::format("saved-checkpoint-{}", levelID);
    }

    void saveFullCheckpointState(PlayLayer* pl, CheckpointObject* cp) {
        if (!pl || !pl->m_level || !cp) return;

        // Serialize the player's spatial state
        auto key = getSaveKey(pl->m_level->m_levelID.value());
        
        Mod::get()->setSavedValue<bool>(key + "-hasSave", true);
        Mod::get()->setSavedValue<float>(key + "-x", cp->m_gameState.m_playerPosition.x);
        Mod::get()->setSavedValue<float>(key + "-y", cp->m_gameState.m_playerPosition.y);
        Mod::get()->setSavedValue<double>(key + "-yvel", cp->m_gameState.m_playerYVelocity);
        Mod::get()->setSavedValue<int>(key + "-attempts", pl->m_attempts);

        // NOTE: For full trigger/item state persistence across game restarts,
        // GD's internal m_effectManager state can be serialized to JSON here.
    }

    void loadFullCheckpointState(PlayLayer* pl) {
        if (!pl || !pl->m_level) return;
        auto key = getSaveKey(pl->m_level->m_levelID.value());

        if (!Mod::get()->getSavedValue<bool>(key + "-hasSave", false)) return;

        // 1. Instantiate a genuine GD CheckpointObject
        auto cp = CheckpointObject::create();
        if (!cp) return;

        // 2. Populate the native GameState structure
        float x = Mod::get()->getSavedValue<float>(key + "-x", 0.f);
        float y = Mod::get()->getSavedValue<float>(key + "-y", 0.f);
        
        cp->m_gameState.m_playerPosition = {x, y};
        cp->m_gameState.m_playerYVelocity = Mod::get()->getSavedValue<double>(key + "-yvel", 0.0);

        // 3. Register the checkpoint with PlayLayer's native array
        if (pl->m_checkpointArray) {
            pl->m_checkpointArray->addObject(cp);
        }

        // 4. Force PlayLayer to execute its native checkpoint restore logic
        // This handles player positioning, object groups, and camera triggers
        pl->loadFromCheckpoint(cp);
    }
}

// 1. Intercept play attempt on LevelInfoLayer
class $modify(MyLevelInfoLayer, LevelInfoLayer) {
    struct Fields {
        bool m_confirmed = false;
    };

    void onPlay(cocos2d::CCObject* sender) {
        if (!m_fields->m_confirmed && m_level && m_level->isPlatformer()) {
            auto key = getSaveKey(m_level->m_levelID.value());

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

// 2. Hook PlayLayer to save & restore authentic native checkpoints
class $modify(LevelStateSave, PlayLayer) {
    CheckpointObject* markCheckpoint() {
        // Let GD create its native CheckpointObject containing GJGameState
        auto cp = PlayLayer::markCheckpoint();
        if (cp && this->m_isPlatformer) {
            saveFullCheckpointState(this, cp);
        }
        return cp;
    }

    void resetLevel() {
        PlayLayer::resetLevel();

        // Restore native checkpoint when starting level if confirmed
        if (g_shouldRestoreCheckpoint) {
            g_shouldRestoreCheckpoint = false;
            loadFullCheckpointState(this);
        }
    }
};