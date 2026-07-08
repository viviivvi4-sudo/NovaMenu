#include "ModMenuPopup.hpp"
#include "BotReplay.hpp"
#include "ModState.hpp"
#include <Geode/binding/FMODAudioEngine.hpp>
#include <cstdlib>
#include <algorithm>

ModMenuPopup* ModMenuPopup::create() {
    auto ret = new ModMenuPopup();
    if (ret->init()) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool ModMenuPopup::init() {
    if (!Popup::init(380.f, 320.f)) return false;

    this->setTitle("Nova Menu");

    // Add our own close (X) button in the top-left corner. We don't touch
    // Popup's own default close button (its exact member name isn't
    // something we could confirm safely) - this is just an extra, clearly
    // visible way to close the popup exactly where it was asked for.
    auto closeMenu = CCMenu::create();
    closeMenu->setPosition({0.f, 0.f});
    addStyledButton(closeMenu, "X", 999, 20.f, m_mainLayer->getContentSize().height - 20.f, 32.f, 32.f);
    m_mainLayer->addChild(closeMenu);

    // --- Tab bar (4 real buttons, not bare text) ---
    m_tabMenu = CCMenu::create();
    m_tabMenu->setPosition({0.f, 0.f});

    const char* names[5] = {"Level", "Player", "Cosmetic", "Bot Replay", "Bypass"};
    float btnWidth = 68.f;
    float startX = m_mainLayer->getContentSize().width / 2 - (btnWidth * 2.f) - 8.f;
    float y = m_mainLayer->getContentSize().height - 65.f;

    for (int i = 0; i < 5; i++) {
        addStyledButton(m_tabMenu, names[i], i, startX + i * (btnWidth + 4.f), y, btnWidth, 28.f);
    }
    m_mainLayer->addChild(m_tabMenu);

    // --- Content container ---
    m_contentLayer = CCNode::create();
    m_contentLayer->setPosition({
        m_mainLayer->getContentSize().width / 2,
        m_mainLayer->getContentSize().height / 2 - 30.f
    });
    m_mainLayer->addChild(m_contentLayer);

    this->selectTab(ModTab::Level);
    return true;
}

CCMenuItemSpriteExtra* ModMenuPopup::addStyledButton(
    CCMenu* menu, std::string const& text, int tag,
    float x, float y, float width, float height,
    char const* bgSprite
) {
    auto bg = CCScale9Sprite::create(bgSprite);
    bg->setContentSize({width, height});

    auto label = CCLabelBMFont::create(text.c_str(), "bigFont.fnt");
    label->setScale(std::min(0.35f, (width - 10.f) / label->getContentSize().width));
    label->setPosition({width / 2.f, height / 2.f});
    bg->addChild(label);

    auto btn = CCMenuItemSpriteExtra::create(bg, this, menu_selector(ModMenuPopup::onButton));
    btn->setTag(tag);
    btn->setPosition({x, y});
    menu->addChild(btn);
    return btn;
}

CCMenuItemToggler* ModMenuPopup::addToggleRow(
    CCMenu* menu, std::string const& label, SEL_MenuHandler handler,
    float x, float y, bool initialState
) {
    auto toggle = CCMenuItemToggler::createWithStandardSprites(this, handler, 0.65f);
    toggle->toggle(initialState);
    toggle->setPosition({x, y});
    menu->addChild(toggle);

    auto lbl = CCLabelBMFont::create(label.c_str(), "bigFont.fnt");
    lbl->setScale(0.35f);
    lbl->setAnchorPoint({1.f, 0.5f});
    lbl->setPosition({x - 22.f, y});
    m_contentLayer->addChild(lbl);

    return toggle;
}

TextInput* ModMenuPopup::addNumberInput(
    std::string const& label, float x, float y, float width,
    float initialValue, float minVal, float maxVal,
    std::function<void(float)> onChange
) {
    auto input = TextInput::create(width, "0.0", "bigFont.fnt");
    input->setPosition({x, y});
    input->setString(fmt::format("{:.2f}", initialValue));

    input->setCallback([onChange, minVal, maxVal](std::string const& text) {
        if (text.empty()) return;
        char* endPtr = nullptr;
        float val = std::strtof(text.c_str(), &endPtr);
        if (endPtr == text.c_str()) return; // not a valid number, ignore
        val = std::clamp(val, minVal, maxVal);
        onChange(val);
    });

    m_contentLayer->addChild(input);

    auto lbl = CCLabelBMFont::create(label.c_str(), "bigFont.fnt");
    lbl->setScale(0.35f);
    lbl->setAnchorPoint({1.f, 0.5f});
    lbl->setPosition({x - width / 2.f - 8.f, y});
    m_contentLayer->addChild(lbl);

    return input;
}

void ModMenuPopup::onButton(CCObject* sender) {
    auto tag = static_cast<CCNode*>(sender)->getTag();
    auto br = BotReplay::get();

    // Tabs
    if (tag >= 0 && tag <= 4) {
        m_showingImportList = false;
        this->selectTab(static_cast<ModTab>(tag));
        return;
    }

    // Import list entries (tag = 1000 + index)
    if (tag >= 1000) {
        size_t idx = static_cast<size_t>(tag - 1000);
        if (idx < m_replayFiles.size()) {
            if (br->importFromFile(m_replayFiles[idx])) {
                Notification::create(
                    fmt::format("Imported '{}' ({} frames)", m_replayFiles[idx], br->frameCount()).c_str(),
                    NotificationIcon::Success
                )->show();
            } else {
                Notification::create("Failed to import replay file.", NotificationIcon::Error)->show();
            }
        }
        m_showingImportList = false;
        this->selectTab(ModTab::BotReplayTab);
        return;
    }

    switch (tag) {
        case 999: // Custom close (X) button
            this->removeFromParentAndCleanup(true);
            return;

        // --- Bot Replay action buttons ---
        case 101: // Stop
            br->stopRecording();
            br->stopPlaying();
            Notification::create(
                fmt::format("Stopped. {} frames recorded.", br->frameCount()).c_str(),
                NotificationIcon::Info
            )->show();
            break;
        case 103: // Reset
            br->reset();
            Notification::create("Replay data cleared.", NotificationIcon::Info)->show();
            break;
        case 104: { // Export
            auto path = br->exportToFile(m_exportName.empty() ? "myreplay" : m_exportName);
            if (path.has_value()) {
                Notification::create(
                    fmt::format("Saved as {}", path->filename().string()).c_str(),
                    NotificationIcon::Success
                )->show();
            } else {
                Notification::create("Nothing to export (record something first).", NotificationIcon::Error)->show();
            }
            break;
        }
        case 105: // Import (open file list)
            m_replayFiles = BotReplay::listReplayFiles();
            m_showingImportList = true;
            break;
        case 106: // Back (from import list)
            m_showingImportList = false;
            break;

        // --- Level tab ---
        case 200: { // Restart Level
            if (auto pl = PlayLayer::get()) {
                pl->resetLevel();
                Notification::create("Level restarted.", NotificationIcon::Success)->show();
            } else {
                Notification::create("Not currently in a level.", NotificationIcon::Error)->show();
            }
            break;
        }
        case 201: { // Show Level Info
            if (auto pl = PlayLayer::get()) {
                auto level = pl->m_level;
                auto text = level
                    ? fmt::format("{} (ID: {})", std::string(level->m_levelName), level->m_levelID)
                    : std::string("Unknown level");
                Notification::create(text.c_str(), NotificationIcon::Info)->show();
            } else {
                Notification::create("Not currently in a level.", NotificationIcon::Error)->show();
            }
            break;
        }

        // --- Cosmetic tab ---
        case 220: { // Random player color
            if (auto pl = PlayLayer::get()) {
                if (pl->m_player1) {
                    auto color = ccColor3B{
                        static_cast<GLubyte>(rand() % 256),
                        static_cast<GLubyte>(rand() % 256),
                        static_cast<GLubyte>(rand() % 256)
                    };
                    pl->m_player1->setColor(color);
                    Notification::create("Player color randomized.", NotificationIcon::Success)->show();
                }
            } else {
                Notification::create("Not currently in a level.", NotificationIcon::Error)->show();
            }
            break;
        }
        case 221: { // Reset player color to white
            if (auto pl = PlayLayer::get()) {
                if (pl->m_player1) {
                    pl->m_player1->setColor({255, 255, 255});
                    Notification::create("Player color reset.", NotificationIcon::Success)->show();
                }
            } else {
                Notification::create("Not currently in a level.", NotificationIcon::Error)->show();
            }
            break;
        }

        // --- Player tab: speed presets removed, now handled by a text
        // input box (see buildPlayerTab / addNumberInput).

        default:
            break;
    }

    // Refresh the current tab so labels/toggles/lists reflect the new state.
    this->selectTab(m_currentTab);
}

void ModMenuPopup::onNoclipToggle(CCObject*) {
    g_noclipEnabled = !g_noclipEnabled;
    Notification::create(
        g_noclipEnabled ? "Noclip enabled." : "Noclip disabled.",
        NotificationIcon::Success
    )->show();
    this->selectTab(m_currentTab);
}

void ModMenuPopup::onRecordRadio(CCObject*) {
    auto br = BotReplay::get();
    if (br->isRecording()) {
        br->stopRecording();
        this->selectTab(m_currentTab);
        return;
    }

    auto pl = PlayLayer::get();
    if (!pl) {
        Notification::create("You need to be inside a level to record.", NotificationIcon::Error)->show();
        return;
    }

    // Force a fixed, high TPS during recording - a variable tick rate is
    // a major source of macro desync (same recorded frame number can
    // still simulate slightly differently if the tick's dt isn't
    // consistent). Locked while recording/playing - see onTpsToggle.
    g_tpsBypassEnabled = true;
    if (g_tpsTarget < 60.f) g_tpsTarget = 240.f;

    pl->resetLevel();
    br->startRecording();
    this->removeFromParentAndCleanup(true);
    Notification::create(
        fmt::format("Level restarted, TPS locked at {:.0f} - resume now!", g_tpsTarget).c_str(),
        NotificationIcon::Success
    )->show();
}

void ModMenuPopup::onPlayRadio(CCObject*) {
    auto br = BotReplay::get();
    if (br->isPlaying()) {
        br->stopPlaying();
        this->selectTab(m_currentTab);
        return;
    }

    if (br->frameCount() == 0) {
        Notification::create("No replay recorded yet!", NotificationIcon::Error)->show();
        return;
    }

    auto pl = PlayLayer::get();
    if (!pl) {
        Notification::create("You need to be inside a level to play a replay.", NotificationIcon::Error)->show();
        return;
    }

    // Same fixed-TPS requirement as recording - the replay was captured
    // at a locked tick rate, so playback needs to match it exactly.
    g_tpsBypassEnabled = true;
    if (g_tpsTarget < 60.f) g_tpsTarget = 240.f;

    pl->resetLevel();
    br->startPlaying();
    this->removeFromParentAndCleanup(true);
    Notification::create(
        fmt::format("Level restarted, TPS locked at {:.0f} - resume now!", g_tpsTarget).c_str(),
        NotificationIcon::Success
    )->show();
}

void ModMenuPopup::onFpsToggle(CCObject*) {
    g_fpsBypassEnabled = !g_fpsBypassEnabled;
    if (!g_fpsBypassEnabled && !g_tpsBypassEnabled && !g_hzBypassEnabled) {
        CCDirector::sharedDirector()->setAnimationInterval(1.0 / 60.0);
    }
    Notification::create(
        g_fpsBypassEnabled ? "FPS bypass enabled (240fps cap)." : "FPS bypass disabled.",
        NotificationIcon::Success
    )->show();
    this->selectTab(m_currentTab);
}

void ModMenuPopup::onTpsToggle(CCObject*) {
    auto br = BotReplay::get();
    if (br->isRecording() || br->isPlaying()) {
        Notification::create(
            "TPS is locked while a Bot Replay recording/playback is active.",
            NotificationIcon::Error
        )->show();
        this->selectTab(m_currentTab); // keep the toggle visually ON
        return;
    }

    g_tpsBypassEnabled = !g_tpsBypassEnabled;
    if (!g_fpsBypassEnabled && !g_tpsBypassEnabled && !g_hzBypassEnabled) {
        CCDirector::sharedDirector()->setAnimationInterval(1.0 / 60.0);
    }
    Notification::create(
        g_tpsBypassEnabled ? "TPS bypass enabled (experimental, 360 ticks)." : "TPS bypass disabled.",
        NotificationIcon::Success
    )->show();
    this->selectTab(m_currentTab);
}

void ModMenuPopup::onHzToggle(CCObject*) {
    g_hzBypassEnabled = !g_hzBypassEnabled;
    if (!g_fpsBypassEnabled && !g_tpsBypassEnabled && !g_hzBypassEnabled) {
        CCDirector::sharedDirector()->setAnimationInterval(1.0 / 60.0);
    }
    Notification::create(
        g_hzBypassEnabled ? "Hz bypass enabled (uncaps engine frame rate)." : "Hz bypass disabled.",
        NotificationIcon::Success
    )->show();
    this->selectTab(m_currentTab);
}

void ModMenuPopup::onSpeedhackToggle(CCObject*) {
    g_speedhackEnabled = !g_speedhackEnabled;
    if (!g_speedhackEnabled) {
        CCDirector::sharedDirector()->getScheduler()->setTimeScale(1.f);
    }
    Notification::create(
        g_speedhackEnabled
            ? fmt::format("Speedhack enabled ({:.2f}x).", g_speedhackValue).c_str()
            : "Speedhack disabled.",
        NotificationIcon::Success
    )->show();
    this->selectTab(m_currentTab);
}

void ModMenuPopup::onSpeedhackMusicToggle(CCObject*) {
    g_speedhackMusicEnabled = !g_speedhackMusicEnabled;
    Notification::create(
        g_speedhackMusicEnabled ? "Music will sync with speedhack." : "Music speed sync disabled.",
        NotificationIcon::Success
    )->show();
    this->selectTab(m_currentTab);
}

void ModMenuPopup::onSafeModeToggle(CCObject*) {
    g_safeModeEnabled = !g_safeModeEnabled;
    Notification::create(
        g_safeModeEnabled ? "Safe Mode enabled." : "Safe Mode disabled.",
        NotificationIcon::Success
    )->show();
    this->selectTab(m_currentTab);
}

void ModMenuPopup::onAutoSafeModeToggle(CCObject*) {
    g_autoSafeModeEnabled = !g_autoSafeModeEnabled;
    Notification::create(
        g_autoSafeModeEnabled ? "Auto Safe Mode enabled." : "Auto Safe Mode disabled.",
        NotificationIcon::Success
    )->show();
    this->selectTab(m_currentTab);
}

void ModMenuPopup::onDisableTrailsToggle(CCObject*) {
    g_disableTrails = !g_disableTrails;
    Notification::create(
        g_disableTrails ? "Trails disabled." : "Trails enabled.",
        NotificationIcon::Success
    )->show();
    this->selectTab(m_currentTab);
}

void ModMenuPopup::onDisableParticlesToggle(CCObject*) {
    g_disableParticles = !g_disableParticles;
    Notification::create(
        g_disableParticles ? "Particles disabled." : "Particles enabled.",
        NotificationIcon::Success
    )->show();
    this->selectTab(m_currentTab);
}

void ModMenuPopup::onDisableEffectsToggle(CCObject*) {
    g_disableEffects = !g_disableEffects;
    Notification::create(
        g_disableEffects ? "Effects disabled." : "Effects enabled.",
        NotificationIcon::Success
    )->show();
    this->selectTab(m_currentTab);
}

void ModMenuPopup::clearContent() {
    m_contentLayer->removeAllChildren();
}

void ModMenuPopup::selectTab(ModTab tab) {
    m_currentTab = tab;
    clearContent();
    switch (tab) {
        case ModTab::Level: buildLevelTab(); break;
        case ModTab::Player: buildPlayerTab(); break;
        case ModTab::Cosmetic: buildCosmeticTab(); break;
        case ModTab::BotReplayTab:
            if (m_showingImportList) buildImportList();
            else buildBotReplayTab();
            break;
        case ModTab::Bypass: buildBypassTab(); break;
    }
}

void ModMenuPopup::buildLevelTab() {
    auto menu = CCMenu::create();
    menu->setPosition({0.f, 0.f});

    addStyledButton(menu, "Restart Level", 200, 0.f, 105.f, 180.f, 30.f);
    addStyledButton(menu, "Show Level Info", 201, 0.f, 68.f, 180.f, 30.f);

    m_contentLayer->addChild(menu);

    auto perfTitle = CCLabelBMFont::create("Performance bypass", "bigFont.fnt");
    perfTitle->setScale(0.38f);
    perfTitle->setPosition({0.f, 35.f});
    m_contentLayer->addChild(perfTitle);

    auto toggleMenu = CCMenu::create();
    toggleMenu->setPosition({0.f, 0.f});
    m_fpsToggle = addToggleRow(toggleMenu, "FPS", menu_selector(ModMenuPopup::onFpsToggle), 20.f, -5.f, g_fpsBypassEnabled);
    m_tpsToggle = addToggleRow(toggleMenu, "TPS", menu_selector(ModMenuPopup::onTpsToggle), 20.f, -35.f, g_tpsBypassEnabled);
    m_hzToggle  = addToggleRow(toggleMenu, "Hz",  menu_selector(ModMenuPopup::onHzToggle),  20.f, -65.f, g_hzBypassEnabled);
    m_contentLayer->addChild(toggleMenu);

    addNumberInput("target", 110.f, -5.f, 70.f, g_fpsTarget, 30.f, 999.f,
        [](float v) { g_fpsTarget = v; });
    addNumberInput("target", 110.f, -35.f, 70.f, g_tpsTarget, 30.f, 999.f,
        [](float v) { g_tpsTarget = v; });
    addNumberInput("target", 110.f, -65.f, 70.f, g_hzTarget, 30.f, 999.f,
        [](float v) { g_hzTarget = v; });

    auto note = CCLabelBMFont::create(
        "Hz/TPS bypass only remove the engine's internal\nframe cap - true display Hz is OS-controlled.",
        "chatFont.fnt"
    );
    note->setScale(0.3f);
    note->setAlignment(kCCTextAlignmentCenter);
    note->setPosition({0.f, -105.f});
    m_contentLayer->addChild(note);
}

void ModMenuPopup::buildPlayerTab() {
    auto menu = CCMenu::create();
    menu->setPosition({0.f, 0.f});

    m_speedhackToggle = addToggleRow(menu, "Speedhack Enabled",
        menu_selector(ModMenuPopup::onSpeedhackToggle), 20.f, 105.f, g_speedhackEnabled);

    addNumberInput("Speed (0.01x-100x)", 130.f, 105.f, 80.f, g_speedhackValue, 0.01f, 100.f,
        [](float v) {
            g_speedhackValue = v;
        });

    m_speedhackMusicToggle = addToggleRow(menu, "Sync Music Speed",
        menu_selector(ModMenuPopup::onSpeedhackMusicToggle), 20.f, 70.f, g_speedhackMusicEnabled);

    m_noclipToggle = addToggleRow(menu, "Noclip (godmode)",
        menu_selector(ModMenuPopup::onNoclipToggle), 20.f, 35.f, g_noclipEnabled);

    m_safeModeToggle = addToggleRow(menu, "Safe Mode",
        menu_selector(ModMenuPopup::onSafeModeToggle), 20.f, 5.f, g_safeModeEnabled);

    m_autoSafeModeToggle = addToggleRow(menu, "Auto Safe Mode",
        menu_selector(ModMenuPopup::onAutoSafeModeToggle), 20.f, -20.f, g_autoSafeModeEnabled);

    m_contentLayer->addChild(menu);

    auto note = CCLabelBMFont::create(
        "Safe Mode clamps speed to 0.1x-3x to avoid physics\nglitches. Auto Safe Mode turns it on only while a\nBot Replay macro is playing back.",
        "chatFont.fnt"
    );
    note->setScale(0.3f);
    note->setAlignment(kCCTextAlignmentCenter);
    note->setPosition({0.f, -60.f});
    m_contentLayer->addChild(note);
}

void ModMenuPopup::buildCosmeticTab() {
    auto menu = CCMenu::create();
    menu->setPosition({0.f, 0.f});

    addStyledButton(menu, "Random Player Color", 220, 0.f, 105.f, 220.f, 30.f);
    addStyledButton(menu, "Reset Player Color", 221, 0.f, 70.f, 220.f, 30.f);

    m_disableTrailsToggle = addToggleRow(menu, "Disable All Trails",
        menu_selector(ModMenuPopup::onDisableTrailsToggle), 20.f, 30.f, g_disableTrails);

    m_disableParticlesToggle = addToggleRow(menu, "Disable Particles",
        menu_selector(ModMenuPopup::onDisableParticlesToggle), 20.f, 5.f, g_disableParticles);

    m_disableEffectsToggle = addToggleRow(menu, "Disable Effects",
        menu_selector(ModMenuPopup::onDisableEffectsToggle), 20.f, -20.f, g_disableEffects);

    m_contentLayer->addChild(menu);

    auto note = CCLabelBMFont::create(
        "\"No Endshake\" isn't included yet - still verifying\na safe way to hook it without risking a crash.",
        "chatFont.fnt"
    );
    note->setScale(0.3f);
    note->setAlignment(kCCTextAlignmentCenter);
    note->setPosition({0.f, -70.f});
    m_contentLayer->addChild(note);
}

void ModMenuPopup::buildBotReplayTab() {
    auto br = BotReplay::get();

    auto status = CCLabelBMFont::create(
        fmt::format("Frames recorded: {}", br->frameCount()).c_str(),
        "bigFont.fnt"
    );
    status->setScale(0.45f);
    status->setPosition({0.f, 105.f});
    m_contentLayer->addChild(status);

    const char* stateText = br->isRecording() ? "Status: RECORDING"
                            : br->isPlaying() ? "Status: PLAYING"
                            : "Status: idle";
    auto stateLabel = CCLabelBMFont::create(stateText, "bigFont.fnt");
    stateLabel->setScale(0.4f);
    stateLabel->setPosition({0.f, 80.f});
    m_contentLayer->addChild(stateLabel);

    auto tip = CCLabelBMFont::create(
        "Normal Mode only for now (not Practice Mode -\ncheckpoint respawns aren't tracked yet).\nPause, tap Record, resume to capture your run.",
        "chatFont.fnt"
    );
    tip->setScale(0.38f);
    tip->setAlignment(kCCTextAlignmentCenter);
    tip->setPosition({0.f, 50.f});
    m_contentLayer->addChild(tip);

    auto radioMenu = CCMenu::create();
    radioMenu->setPosition({0.f, 0.f});
    m_recordRadio = addToggleRow(radioMenu, "Record", menu_selector(ModMenuPopup::onRecordRadio), 130.f, 20.f, br->isRecording());
    m_playRadio   = addToggleRow(radioMenu, "Play",   menu_selector(ModMenuPopup::onPlayRadio),   130.f, -5.f, br->isPlaying());
    m_contentLayer->addChild(radioMenu);

    auto menu = CCMenu::create();
    menu->setPosition({0.f, 0.f});

    // Export filename input
    m_exportNameInput = TextInput::create(180.f, "filename", "bigFont.fnt");
    m_exportNameInput->setPosition({0.f, -105.f});
    m_exportNameInput->setString(m_exportName);
    m_exportNameInput->setCallback([this](std::string const& text) {
        m_exportName = text;
    });
    m_contentLayer->addChild(m_exportNameInput);

    float bw = 90.f, bh = 30.f;
    addStyledButton(menu, "Stop", 101, -100.f, -35.f, bw, bh);
    addStyledButton(menu, "Reset", 103, 0.f, -35.f, bw, bh);
    addStyledButton(menu, "Export", 104, -100.f, -70.f, bw, bh);
    addStyledButton(menu, "Import", 105, 0.f, -70.f, bw, bh);

    m_contentLayer->addChild(menu);
}

void ModMenuPopup::buildBypassTab() {
    auto title = CCLabelBMFont::create("Bypass Tools", "bigFont.fnt");
    title->setScale(0.45f);
    title->setPosition({0.f, 105.f});
    m_contentLayer->addChild(title);

    // Being upfront: these need verified hooks into GD's icon/color/level
    // systems before they can be wired up safely, so they're listed here
    // as a roadmap rather than pretending they already work.
    auto note = CCLabelBMFont::create(
        "Planned for a future update:\n\n"
        "- Character limit bypass (name/desc/comment)\n"
        "- Icon & color unlock\n"
        "- Level copy password bypass\n"
        "- Music/song customizer\n\n"
        "These touch deeper GD internals that need more\n"
        "verification first, to avoid shipping something\n"
        "that crashes on your device.",
        "chatFont.fnt"
    );
    note->setScale(0.34f);
    note->setAlignment(kCCTextAlignmentCenter);
    note->setPosition({0.f, -10.f});
    m_contentLayer->addChild(note);
}

void ModMenuPopup::buildImportList() {
    auto title = CCLabelBMFont::create("Select a replay to import:", "bigFont.fnt");
    title->setScale(0.42f);
    title->setPosition({0.f, 105.f});
    m_contentLayer->addChild(title);

    auto menu = CCMenu::create();
    menu->setPosition({0.f, 0.f});

    if (m_replayFiles.empty()) {
        auto none = CCLabelBMFont::create("No saved .vdr2 replays found.", "chatFont.fnt");
        none->setScale(0.45f);
        none->setPosition({0.f, 60.f});
        m_contentLayer->addChild(none);
    } else {
        float y = 90.f;
        for (size_t i = 0; i < m_replayFiles.size() && i < 6; i++) {
            addStyledButton(menu, m_replayFiles[i], static_cast<int>(1000 + i), 0.f, y, 220.f, 26.f);
            y -= 30.f;
        }
    }

    addStyledButton(menu, "Back", 106, 0.f, -100.f, 90.f, 28.f);
    m_contentLayer->addChild(menu);
}
