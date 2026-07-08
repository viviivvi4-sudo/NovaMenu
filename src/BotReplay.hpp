#pragma once
#include <Geode/Geode.hpp>
#include <vector>
#include <string>
#include <cstdint>
#include <fstream>
#include <filesystem>

using namespace geode::prelude;

// One recorded input event: at which physics frame, which button,
// pressed or released, on which player (1 = p1, 2 = p2/dual).
struct ReplayFrame {
    int frame;
    int button;   // 1 = jump, 2 = left, 3 = right
    bool player2;
    bool down;    // true = push, false = release
};

// Simple frame-perfect macro recorder + player for a single attempt.
// Works by hooking PlayerObject::pushButton / releaseButton while recording
// (captures real input) and PlayLayer::update while playing back
// (re-fires the same button calls at the same physics frame count).
//
// Replay files use a custom ".vdr2" format:
//   [4 bytes] magic "VDR2"
//   [4 bytes] format version (uint32)
//   [8 bytes] frame count (uint64)
//   [N * sizeof(ReplayFrame)] frame data
class BotReplay {
public:
    static BotReplay* get();

    void startRecording();
    void stopRecording();
    void startPlaying();
    void stopPlaying();

    void reset();

    bool isRecording() const { return m_recording; }
    bool isPlaying() const { return m_playing; }

    // Called every physics tick from PlayLayer::update hook
    void tick(int frame);

    // Called from PlayerObject push/release hooks while recording
    void onInput(int button, bool player2, bool down, int frame);

    size_t frameCount() const { return m_frames.size(); }

    // --- .vdr2 file management ---

    // Directory where replay files live: <mod save dir>/replays/
    static std::filesystem::path getReplayDir();

    // Saves the currently recorded frames to <name>.vdr2. Returns the
    // full path on success, or std::nullopt on failure.
    std::optional<std::filesystem::path> exportToFile(std::string const& name);

    // Loads a .vdr2 file (by filename, with or without extension) into
    // the current buffer, replacing whatever was recorded before.
    bool importFromFile(std::string const& name);

    // Auto-generates a timestamped filename ("replay_YYYYMMDD_HHMMSS")
    // and exports to it. Returns the full path on success.
    std::optional<std::filesystem::path> exportAuto();

    // Lists all *.vdr2 files currently in the replay directory (filenames
    // only, without the folder path).
    static std::vector<std::string> listReplayFiles();

private:
    std::vector<ReplayFrame> m_frames;
    size_t m_playIndex = 0;
    bool m_recording = false;
    bool m_playing = false;

    // Frames are stored/compared RELATIVE to when recording/playback
    // started, not as raw absolute frame counters. Without this, if you
    // pause partway through a run (so the absolute frame counter is
    // already high) and then play back, every recorded input would look
    // "already due" and fire all at once on the very first tick instead
    // of being spaced out like the original run.
    int m_recordOriginFrame = 0;
    bool m_recordOriginSet = false;
    int m_playOriginFrame = 0;
    bool m_playOriginSet = false;

    bool saveToFile(const std::filesystem::path& path);
    bool loadFromFile(const std::filesystem::path& path);
};
