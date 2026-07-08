#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include "BotReplay.hpp"
#include "ModState.hpp"
#include <algorithm>

using namespace geode::prelude;

// Track physics frame count ourselves (GD's internal frame counter isn't
// always exposed the same way across versions, so we keep our own).
static int g_frameCount = 0;

static void applyCosmeticToggles(PlayerObject* p) {
    if (!p) return;

    if (p->m_regularTrail) p->m_regularTrail->setVisible(!g_disableTrails);
    if (p->m_shipStreak) p->m_shipStreak->setVisible(!g_disableTrails);
    if (p->m_waveTrail) p->m_waveTrail->setVisible(!g_disableTrails);

    p->m_hasGroundParticles = !g_disableParticles;
    p->m_hasShipParticles = !g_disableParticles;
    p->m_playEffects = !g_disableEffects;
}

class $modify(BRPlayLayer, PlayLayer) {
    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;
        g_frameCount = 0;

        // Fresh attempt: if we're about to play back a macro, rewind it.
        if (BotReplay::get()->isPlaying()) {
            BotReplay::get()->stopPlaying();
            BotReplay::get()->startPlaying();
        }
        return true;
    }

    void update(float dt) {
        // Best-effort FPS/Hz/TPS bypass: raise the engine's animation
        // interval (frames-per-second cap) if any bypass toggle is on.
        // TPS bypass takes priority since it also implies wanting a
        // faster physics loop.
        if (g_tpsBypassEnabled) {
            CCDirector::sharedDirector()->setAnimationInterval(1.0 / g_tpsTarget);
        } else if (g_fpsBypassEnabled) {
            CCDirector::sharedDirector()->setAnimationInterval(1.0 / g_fpsTarget);
        } else if (g_hzBypassEnabled) {
            CCDirector::sharedDirector()->setAnimationInterval(1.0 / g_hzTarget);
        }

        // Speedhack: scale the actual delta-time fed into PlayLayer's own
        // update (this is what really drives GD's gameplay physics speed -
        // CCScheduler's timeScale property mostly only affects generic
        // CCActions, not GD's internal fixed-step gameplay loop).
        float speed = g_speedhackEnabled ? g_speedhackValue : 1.f;

        // Safe Mode clamps the effective speed to avoid physics weirdness
        // at extreme values. Auto Safe Mode kicks in automatically while
        // a macro is actively being played back.
        bool safeModeActive = g_safeModeEnabled ||
            (g_autoSafeModeEnabled && BotReplay::get()->isPlaying());
        if (safeModeActive) {
            speed = std::clamp(speed, 0.1f, 3.f);
        }

        applyCosmeticToggles(m_player1);
        applyCosmeticToggles(m_player2);

        PlayLayer::update(dt * speed);
    }

    void resetLevel() {
        PlayLayer::resetLevel();
        g_frameCount = 0;
        if (BotReplay::get()->isPlaying()) {
            // restart macro from the beginning on every retry
            BotReplay::get()->startPlaying();
        }
    }
};

// THIS is the real fixed-rate physics tick (confirmed straight from
// xdBot's own source: it hooks this exact function for frame counting
// and macro playback, NOT PlayLayer::update). update() only runs once
// per rendered frame, which is a different (variable) rate than GD's
// actual physics step - using update() for frame counting was the root
// cause of playback being desynced.
class $modify(BRBaseGameLayer, GJBaseGameLayer) {
    void processCommands(float dt, bool isHalfTick, bool isLastTick) {
        // GD can run "half tick" sub-steps for smoother collision at
        // certain speeds. We only want to count/inject on full, real
        // ticks - otherwise our frame counter wouldn't match a stable
        // "one frame = one recorded slot" meaning.
        if (!isHalfTick && PlayLayer::get()) {
            g_frameCount++;
            BotReplay::get()->tick(g_frameCount);
        }

        GJBaseGameLayer::processCommands(dt, isHalfTick, isLastTick);
    }

    void handleButton(bool down, int button, bool isPlayer1) {
        GJBaseGameLayer::handleButton(down, button, isPlayer1);

        // Always log this, regardless of recording state, so it's easy to
        // confirm via logcat/console whether this hook is firing at all.
        log::info("[FloatingMenuMod] handleButton down={} button={} isPlayer1={} recording={}",
            down, button, isPlayer1, BotReplay::get()->isRecording());

        if (BotReplay::get()->isRecording()) {
            BotReplay::get()->onInput(button, !isPlayer1, down, g_frameCount);
        }
    }
};
