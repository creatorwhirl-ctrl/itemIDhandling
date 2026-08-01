#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>
#include <Geode/binding/GameObject.hpp>
#include <Geode/binding/CheckpointObject.hpp>
#include <Geode/binding/PlayerCheckpoint.hpp>
#include <Geode/ui/Popup.hpp>
#include <sabe.persistenceapi/include/PersistenceAPI.hpp>
#include <filesystem>

using namespace geode::prelude;
using namespace persistenceAPI;

namespace {
    bool g_shouldRestoreCheckpoint = false;

    std::filesystem::path getSaveFilePath(int levelID) {
        auto dir = Mod::get()->getSaveDir();
        std::filesystem::create_directories(dir);
        return dir / fmt::format("checkpoint-{}.bin", levelID);
    }

    // Everything needed for a full resume: player physics state,
    // active colors/effects, and score/item/attempt state.
    void writeCheckpoint(CheckpointObject* cp, Stream& stream) {
        reinterpret_cast<PAPlayerCheckpoint*>(cp->m_player1Checkpoint)->save(stream);
        reinterpret_cast<PAEffectManagerState*>(&cp->m_effectManagerState)->save(stream);
        reinterpret_cast<PAGJGameState*>(&cp->m_gameState)->save(stream);
        stream << cp->m_physicalCheckpointObject->m_startPosition;
    }

    void readCheckpoint(CheckpointObject* cp, Stream& stream) {
        cp->m_player1Checkpoint = PlayerCheckpoint::create();
        CC_SAFE_RETAIN(cp->m_player1Checkpoint);
        geode::log::info("readCheckpoint: loading player checkpoint");
        reinterpret_cast<PAPlayerCheckpoint*>(cp->m_player1Checkpoint)->load(stream);

        geode::log::info("readCheckpoint: loading effect manager state");
        reinterpret_cast<PAEffectManagerState*>(&cp->m_effectManagerState)->load(stream);

        geode::log::info("readCheckpoint: loading game state");
        reinterpret_cast<PAGJGameState*>(&cp->m_gameState)->load(stream);

        geode::log::info("readCheckpoint: reading position");
        cocos2d::CCPoint pos;
        stream >> pos;

        geode::log::info("readCheckpoint: creating marker");
        auto marker = GameObject::createWithFrame("square_01_001.png");
        CC_SAFE_RETAIN(marker);
        marker->m_objectID = 0x2c;
        marker->setOpacity(0);
        marker->setStartPos(pos);
        cp->m_physicalCheckpointObject = marker;
        geode::log::info("readCheckpoint: done");
    }

    std::string getSaveKey(int levelID) {
        return fmt::format("checkpoint-data-{}", levelID);
    }

    void saveCheckpointData(PlayLayer* pl, CheckpointObject* cp) {
        if (!pl || !pl->m_level || !cp || !cp->m_player1Checkpoint) return;

        Stream stream;
        auto path = getSaveFilePath(pl->m_level->m_levelID.value());
        if (!stream.setFile(path.string(), true)) return;

        stream.seek(0);
        stream.clear();
        writeCheckpoint(cp, stream);
        stream.end();

        // Small flag so LevelInfoLayer knows a save exists without opening the file.
        Mod::get()->setSavedValue<bool>(getSaveKey(pl->m_level->m_levelID.value()) + "-exists", true);
    }

    void loadCheckpointData(PlayLayer* pl) {
        if (!pl || !pl->m_level || !pl->m_player1) return;

        auto path = getSaveFilePath(pl->m_level->m_levelID.value());
        if (!std::filesystem::exists(path)) return;

        Stream stream;
        if (!stream.setFile(path.string(), false)) return;
        stream.seek(0);   // <-- add this, mirrors the save path

        auto cp = CheckpointObject::create();
        if (!cp) return;

        readCheckpoint(cp, stream);
        stream.end();

        pl->m_checkpointArray->addObject(cp);
        pl->m_player1->setStartPos(cp->m_player1Checkpoint->m_position);
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
        if (g_shouldRestoreCheckpoint && !m_fields->m_isRestoring) {
            g_shouldRestoreCheckpoint = false;
            m_fields->m_isRestoring = true;
            loadCheckpointData(this);
        }

        PlayLayer::resetLevel();

        m_fields->m_isRestoring = false;
    }
};