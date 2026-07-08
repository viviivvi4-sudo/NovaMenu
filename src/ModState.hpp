#pragma once

// Small shared state used across hook files + the popup UI.
// Kept header-only with inline variables (C++17) to avoid extra .cpp files.
inline bool g_noclipEnabled = false;

// Performance bypass toggles + user-configurable target values.
// NOTE: true display refresh-rate (Hz) negotiation is an OS/Android-level
// setting not reachable from this layer - "Hz bypass" here just removes
// the engine's own frame cap so high-refresh screens aren't held back by
// it. TPS bypass pushes the physics/update loop rate higher too; this is
// experimental since GD's physics were tuned around a fixed step.
inline bool g_fpsBypassEnabled = false;
inline bool g_tpsBypassEnabled = false;
inline bool g_hzBypassEnabled = false;

inline float g_fpsTarget = 240.f;
inline float g_tpsTarget = 360.f;
inline float g_hzTarget = 240.f;

// Speedhack: scales the dt fed into PlayLayer::update directly (this is
// what actually speeds up GD's gameplay physics - CCScheduler's own
// timeScale property mostly only affects generic CCActions, not GD's
// internal fixed-step gameplay loop).
inline bool g_speedhackEnabled = false;
inline float g_speedhackValue = 1.f; // clamp range: 0.01x - 100x

// UI toggle for syncing music speed with speedhack. Currently just
// stored (not wired to FMOD yet - see ReplayHooks.cpp comment for why).
inline bool g_speedhackMusicEnabled = false;

// Safe Mode: clamps the effective speed to a much safer range to avoid
// physics/collision weirdness (tunneling, missed triggers) at extreme
// speeds. Auto Safe Mode turns this on automatically while a Bot Replay
// macro is playing back (the riskiest combo: fast playback + high speed).
inline bool g_safeModeEnabled = false;
inline bool g_autoSafeModeEnabled = false;

// Cosmetic toggles - all backed by confirmed real PlayerObject members
// (m_regularTrail / m_shipStreak / m_waveTrail / m_hasGroundParticles /
// m_hasShipParticles / m_playEffects), reapplied every frame since the
// player object can be recreated on level restart.
inline bool g_disableTrails = false;
inline bool g_disableParticles = false;
inline bool g_disableEffects = false;
