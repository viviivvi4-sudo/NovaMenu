#pragma once
#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/TextInput.hpp>
#include <vector>
#include <string>

using namespace geode::prelude;

enum class ModTab {
    Level = 0,
    Player = 1,
    Cosmetic = 2,
    BotReplayTab = 3
};

// NOTE: in current Geode (v5.7.1), geode::Popup is a plain class, not a
// template. You override init(...) directly (calling Popup::init(w, h)
// first), there is no separate setup() step.
class ModMenuPopup : public geode::Popup {
protected:
    CCMenu* m_tabMenu = nullptr;
    CCNode* m_contentLayer = nullptr;
    ModTab m_currentTab = ModTab::Level;

    // When true, the Bot Replay tab shows a list of saved .vdr2 files to
    // import instead of the normal controls.
    bool m_showingImportList = false;
    std::vector<std::string> m_replayFiles;

    // Live-updating toggles kept as members so we can update them
    // directly instead of rebuilding the whole tab.
    CCMenuItemToggler* m_noclipToggle = nullptr;
    CCMenuItemToggler* m_recordRadio = nullptr;
    CCMenuItemToggler* m_playRadio = nullptr;
    CCMenuItemToggler* m_fpsToggle = nullptr;
    CCMenuItemToggler* m_tpsToggle = nullptr;
    CCMenuItemToggler* m_hzToggle = nullptr;
    CCMenuItemToggler* m_speedhackToggle = nullptr;
    CCMenuItemToggler* m_speedhackMusicToggle = nullptr;
    CCMenuItemToggler* m_safeModeToggle = nullptr;
    CCMenuItemToggler* m_autoSafeModeToggle = nullptr;
    CCMenuItemToggler* m_disableTrailsToggle = nullptr;
    CCMenuItemToggler* m_disableParticlesToggle = nullptr;
    CCMenuItemToggler* m_disableEffectsToggle = nullptr;
    TextInput* m_exportNameInput = nullptr;
    std::string m_exportName = "myreplay";

    bool init() override;

    void selectTab(ModTab tab);
    void onButton(CCObject* sender);
    void onNoclipToggle(CCObject* sender);
    void onRecordRadio(CCObject* sender);
    void onPlayRadio(CCObject* sender);
    void onFpsToggle(CCObject* sender);
    void onTpsToggle(CCObject* sender);
    void onHzToggle(CCObject* sender);
    void onSpeedhackToggle(CCObject* sender);
    void onSpeedhackMusicToggle(CCObject* sender);
    void onSafeModeToggle(CCObject* sender);
    void onAutoSafeModeToggle(CCObject* sender);
    void onDisableTrailsToggle(CCObject* sender);
    void onDisableParticlesToggle(CCObject* sender);
    void onDisableEffectsToggle(CCObject* sender);

    void buildLevelTab();
    void buildPlayerTab();
    void buildCosmeticTab();
    void buildBotReplayTab();
    void buildImportList();

    void clearContent();

    // Creates a proper-looking button (sprite background + label) instead
    // of a bare label, and adds it to the given menu.
    CCMenuItemSpriteExtra* addStyledButton(
        CCMenu* menu, std::string const& text, int tag,
        float x, float y, float width, float height,
        char const* bgSprite = "GJ_button_01.png"
    );

    // Creates a labeled checkbox-style toggle (used as our "radio button"
    // and on/off switches) at the given position.
    CCMenuItemToggler* addToggleRow(
        CCMenu* menu, std::string const& label, SEL_MenuHandler handler,
        float x, float y, bool initialState
    );

    // Creates a small numeric text input box with a label to its left.
    // onChange receives the parsed float (clamped to [minVal, maxVal]);
    // invalid/empty input is ignored (keeps the previous value).
    TextInput* addNumberInput(
        std::string const& label, float x, float y, float width,
        float initialValue, float minVal, float maxVal,
        std::function<void(float)> onChange
    );

public:
    static ModMenuPopup* create();
};
