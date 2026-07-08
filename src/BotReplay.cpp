#include "BotReplay.hpp"
#include <ctime>
#include <cstring>

BotReplay* BotReplay::get() {
    static BotReplay instance;
    return &instance;
}

void BotReplay::reset() {
    m_frames.clear();
    m_playIndex = 0;
    m_recording = false;
    m_playing = false;
    m_recordOriginFrame = 0;
    m_recordOriginSet = false;
    m_playOriginFrame = 0;
    m_playOriginSet = false;
}

void BotReplay::startRecording() {
    m_frames.clear();
    m_recording = true;
    m_playing = false;
    m_recordOriginFrame = 0;
    m_recordOriginSet = false; // set on the first tick() call after this
}

void BotReplay::stopRecording() {
    m_recording = false;
}

void BotReplay::startPlaying() {
    if (m_frames.empty()) {
        Notification::create("No replay recorded yet!", NotificationIcon::Error)->show();
        return;
    }
    m_playIndex = 0;
    m_playing = true;
    m_recording = false;
    m_playOriginSet = false; // set on the first tick() call after this
}

void BotReplay::stopPlaying() {
    m_playing = false;
}

void BotReplay::onInput(int button, bool player2, bool down, int frame) {
    if (!m_recording) return;

    // m_recordOriginFrame is established by tick() at the moment
    // recording actually starts (i.e. the instant you resume the level),
    // NOT by the first button press. Using the first press as origin was
    // the bug: it silently deleted any delay before your first press, so
    // playback fired that first input immediately instead of waiting the
    // same amount of time you actually did.
    m_frames.push_back({frame - m_recordOriginFrame, button, player2, down});
}

void BotReplay::tick(int frame) {
    // Establish the recording origin on the very first tick after
    // recording starts - this is the moment gameplay actually resumes,
    // which is the correct "time zero" reference for the recording.
    if (m_recording && !m_recordOriginSet) {
        m_recordOriginFrame = frame;
        m_recordOriginSet = true;
    }

    if (!m_playing) return;

    auto playLayer = PlayLayer::get();
    if (!playLayer) return;

    if (!m_playOriginSet) {
        m_playOriginFrame = frame;
        m_playOriginSet = true;
    }
    int relFrame = frame - m_playOriginFrame;

    // Fire every recorded event scheduled for this frame (there can be
    // more than one input on the same physics tick). We replay through
    // handleButton (inherited from GJBaseGameLayer) since that's the same
    // single entry point real input goes through - this is more faithful
    // than calling PlayerObject::pushButton/releaseButton directly.
    while (m_playIndex < m_frames.size() && m_frames[m_playIndex].frame <= relFrame) {
        auto& ev = m_frames[m_playIndex];
        playLayer->handleButton(ev.down, ev.button, !ev.player2);
        m_playIndex++;
    }

    // Reached the end of the macro -> stop automatically.
    if (m_playIndex >= m_frames.size()) {
        m_playing = false;
    }
}

// --- .vdr2 file format ---

static constexpr char kMagic[4] = {'V', 'D', 'R', '2'};
static constexpr uint32_t kFormatVersion = 1;

std::filesystem::path BotReplay::getReplayDir() {
    auto dir = Mod::get()->getSaveDir() / "replays";
    if (!std::filesystem::exists(dir)) {
        std::error_code ec;
        std::filesystem::create_directories(dir, ec);
    }
    return dir;
}

bool BotReplay::saveToFile(const std::filesystem::path& path) {
    std::ofstream out(path, std::ios::binary);
    if (!out.is_open()) return false;

    uint64_t count = static_cast<uint64_t>(m_frames.size());

    out.write(kMagic, sizeof(kMagic));
    out.write(reinterpret_cast<const char*>(&kFormatVersion), sizeof(kFormatVersion));
    out.write(reinterpret_cast<const char*>(&count), sizeof(count));
    if (count > 0) {
        out.write(reinterpret_cast<const char*>(m_frames.data()), count * sizeof(ReplayFrame));
    }
    return out.good();
}

bool BotReplay::loadFromFile(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in.is_open()) return false;

    char magic[4];
    in.read(magic, sizeof(magic));
    if (!in || std::memcmp(magic, kMagic, sizeof(kMagic)) != 0) {
        return false; // not a valid .vdr2 file
    }

    uint32_t version = 0;
    in.read(reinterpret_cast<char*>(&version), sizeof(version));
    if (!in || version != kFormatVersion) {
        return false; // unsupported version
    }

    uint64_t count = 0;
    in.read(reinterpret_cast<char*>(&count), sizeof(count));
    if (!in) return false;

    m_frames.resize(static_cast<size_t>(count));
    if (count > 0) {
        in.read(reinterpret_cast<char*>(m_frames.data()), count * sizeof(ReplayFrame));
    }
    return static_cast<bool>(in) || in.eof();
}

std::optional<std::filesystem::path> BotReplay::exportToFile(std::string const& name) {
    auto dir = getReplayDir();
    auto filename = name;
    if (filename.size() < 5 || filename.substr(filename.size() - 5) != ".vdr2") {
        filename += ".vdr2";
    }
    auto path = dir / filename;

    if (!saveToFile(path)) return std::nullopt;
    return path;
}

std::optional<std::filesystem::path> BotReplay::exportAuto() {
    std::time_t t = std::time(nullptr);
    std::tm tmVal{};
#if defined(_WIN32)
    localtime_s(&tmVal, &t);
#else
    localtime_r(&t, &tmVal);
#endif
    char buf[64];
    std::strftime(buf, sizeof(buf), "replay_%Y%m%d_%H%M%S", &tmVal);
    return exportToFile(buf);
}

bool BotReplay::importFromFile(std::string const& name) {
    auto dir = getReplayDir();
    auto filename = name;
    if (filename.size() < 5 || filename.substr(filename.size() - 5) != ".vdr2") {
        filename += ".vdr2";
    }
    return loadFromFile(dir / filename);
}

std::vector<std::string> BotReplay::listReplayFiles() {
    std::vector<std::string> out;
    auto dir = getReplayDir();

    std::error_code ec;
    for (auto& entry : std::filesystem::directory_iterator(dir, ec)) {
        if (!entry.is_regular_file()) continue;
        if (entry.path().extension() == ".vdr2") {
            out.push_back(entry.path().filename().string());
        }
    }
    return out;
}
