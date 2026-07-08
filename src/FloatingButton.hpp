#pragma once
#include <Geode/Geode.hpp>

using namespace geode::prelude;

// A small circular button that floats on top of every scene, can be
// dragged anywhere on screen, and opens the mod menu popup on tap
// (tap = press+release without significant movement; drag = press+move).
//
// Uses CCLayer (not a plain CCNode + manual touch dispatcher registration)
// specifically because CCLayer's setTouchEnabled/onEnter/onExit already
// reliably ties touch registration to the node's actual lifecycle. Manual
// CCDirector::getTouchDispatcher()->addTargetedDelegate/removeDelegate
// calls turned out to leave stale "ghost" touch areas behind whenever a
// layer like PauseLayer got recreated (paused/unpaused repeatedly),
// causing invisible-but-still-clickable buttons.
class FloatingButton : public CCLayer {
protected:
    CCPoint m_pressPos;
    bool m_dragging = false;
    bool m_touchMoved = false;

    bool init() override;
    void onTap();
    void bringToFront(float dt);

public:
    static FloatingButton* create();

    void onEnter() override;
    bool ccTouchBegan(CCTouch* touch, CCEvent* event) override;
    void ccTouchMoved(CCTouch* touch, CCEvent* event) override;
    void ccTouchEnded(CCTouch* touch, CCEvent* event) override;
};
