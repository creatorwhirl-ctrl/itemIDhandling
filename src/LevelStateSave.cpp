#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>
#include <Geode/binding/GameObject.hpp>
#include <Geode/binding/CheckpointObject.hpp>
#include <Geode/binding/PlayerCheckpoint.hpp>
#include <Geode/ui/Popup.hpp>

using namespace geode::prelude;

namespace {
    bool g_shouldRestoreCheckpoint = false;

    std::string getSaveKey(int levelID) {
        return fmt::format("checkpoint-data-{}", levelID);
    }

    // Helper: read the CURRENT resolved color for a channel ID.
    // Verify this against your build with a log::info before trusting it blindly.
    cocos2d::ccColor3B getCurrentChannelColor(PlayLayer* pl, int channelID, cocos2d::ccColor3B fallback) {
        if (!pl->m_colorKeyDict) return fallback;
        auto keyObj = static_cast<CCInteger*>(pl->m_colorKeyDict->objectForKey(std::to_string(channelID)));
        if (!keyObj) return fallback;
        int key = keyObj->getValue();
        if (key < 0 || key >= static_cast<int>(pl->m_keyColors.size())) return fallback;
        return pl->m_keyColors[key];
    }

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
        Mod::get()->setSavedValue<int>(key + "-attempts", pl->m_attempts);

        // Colors: save the two standard channels. Add more IDs here if your
        // level recolors other channels (line, P1, P2, 3DL, custom IDs, etc).
        auto bg = getCurrentChannelColor(pl, 1000, {255,255,255});
        auto ground = getCurrentChannelColor(pl, 1001, {255,255,255});
        Mod::get()->setSavedValue<int>(key + "-bg-r", bg.r);
        Mod::get()->setSavedValue<int>(key + "-bg-g", bg.g);
        Mod::get()->setSavedValue<int>(key + "-bg-b", bg.b);
        Mod::get()->setSavedValue<int>(key + "-gnd-r", ground.r);
        Mod::get()->setSavedValue<int>(key + "-gnd-g", ground.g);
        Mod::get()->setSavedValue<int>(key + "-gnd-b", ground.b);
    }

    void loadCheckpointData(PlayLayer* pl) {
        if (!pl || !pl->m_level || !pl->m_player1) return;

        auto key = getSaveKey(pl->m_level->m_levelID.value());
        if (!Mod::get()->getSavedValue<bool>(key + "-exists", false)) return;

        // Build the checkpoint from a REAL live snapshot, not from scratch,
        // so every field is valid and only the ones we care about get overridden.
        auto cp = pl->markCheckpoint();
        if (!cp || !cp->m_player1Checkpoint) return;
        auto pc = cp->m_player1Checkpoint;

        cocos2d::CCPoint pos = {
            Mod::get()->getSavedValue<float>(key + "-x", pc->m_position.x),
            Mod::get()->getSavedValue<float>(key + "-y", pc->m_position.y)
        };

        pc->m_position     = pos;
        pc->m_yVelocity    = Mod::get()->getSavedValue<double>(key + "-yvel", pc->m_yVelocity);
        pc->m_rotation     = Mod::get()->getSavedValue<float>(key + "-rot", pc->m_rotation);
        pc->m_gravityMod   = Mod::get()->getSavedValue<float>(key + "-gravity", pc->m_gravityMod);
        pc->m_isUpsideDown = Mod::get()->getSavedValue<bool>(key + "-upsidedown", false);
        pc->m_isMini       = Mod::get()->getSavedValue<bool>(key + "-mini", false);
        pc->m_isShip       = Mod::get()->getSavedValue<bool>(key + "-ship", false);
        pc->m_isBall       = Mod::get()->getSavedValue<bool>(key + "-ball", false);
        pc->m_isBird       = Mod::get()->getSavedValue<bool>(key + "-bird", false);
        pc->m_isDart       = Mod::get()->getSavedValue<bool>(key + "-dart", false);
        pc->m_isRobot      = Mod::get()->getSavedValue<bool>(key + "-robot", false);
        pc->m_isSpider     = Mod::get()->getSavedValue<bool>(key + "-spider", false);
        pc->m_isSwing      = Mod::get()->getSavedValue<bool>(key + "-swing", false);

        pl->m_attempts = Mod::get()->getSavedValue<int>(key + "-attempts", pl->m_attempts);

        // Register into the game's own checkpoint system + set start pos,
        // instead of loadFromCheckpoint().
        pl->m_checkpointArray->addObject(cp);
        pl->m_player1->setStartPos(pos);

        // Reapply the colors that were active when this checkpoint was saved.
        cocos2d::ccColor3B bg = {
            static_cast<GLubyte>(Mod::get()->getSavedValue<int>(key + "-bg-r", 255)),
            static_cast<GLubyte>(Mod::get()->getSavedValue<int>(key + "-bg-g", 255)),
            static_cast<GLubyte>(Mod::get()->getSavedValue<int>(key + "-bg-b", 255))
        };
        cocos2d::ccColor3B ground = {
            static_cast<GLubyte>(Mod::get()->getSavedValue<int>(key + "-gnd-r", 255)),
            static_cast<GLubyte>(Mod::get()->getSavedValue<int>(key + "-gnd-g", 255)),
            static_cast<GLubyte>(Mod::get()->getSavedValue<int>(key + "-gnd-b", 255))
        };
        pl->colorObject(1000, bg);
        pl->colorObject(1001, ground);
    }
}

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