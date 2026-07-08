#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include "ModState.hpp"

using namespace geode::prelude;

// Classic "godmode" style noclip: whenever the game tries to kill the
// player, just ignore the call entirely if noclip is enabled. This is a
// simple, well-tested technique that avoids touching collision detection
// internals directly.
class $modify(NoclipPlayLayer, PlayLayer) {
    void destroyPlayer(PlayerObject* player, GameObject* obj) {
        if (g_noclipEnabled) {
            return; // swallow the death, player keeps going
        }
        PlayLayer::destroyPlayer(player, obj);
    }
};
