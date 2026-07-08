#include "FloatingButton.hpp"
#include "ModMenuPopup.hpp"

FloatingButton* FloatingButton::create() {
    auto ret = new FloatingButton();
    if (ret->init()) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool FloatingButton::init() {
    if (!CCLayer::init()) return false;

    float radius = 22.f;
    this->setContentSize({radius * 2.f, radius * 2.f});
    this->setAnchorPoint({0.5f, 0.5f});
    this->ignoreAnchorPointForPosition(false);

    // Small round button drawn directly (no dependency on any specific
    // GD sprite frame name - guaranteed to render regardless of texture
    // pack/version differences).
    auto circle = CCDrawNode::create();
    circle->setContentSize({radius * 2.f, radius * 2.f});
    circle->drawDot({radius, radius}, radius, ccc4f(0.13f, 0.13f, 0.16f, 0.92f));
    circle->drawDot({radius, radius}, radius - 2.5f, ccc4f(0.35f, 0.55f, 0.95f, 1.f));
    this->addChild(circle);

    auto label = CCLabelBMFont::create("M", "bigFont.fnt");
    label->setScale(0.6f);
    label->setPosition({radius, radius});
    this->addChild(label);

    // Default spot: right-middle of the screen.
    auto winSize = CCDirector::sharedDirector()->getWinSize();
    this->setPosition({winSize.width - radius - 10.f, winSize.height / 2});

    // CCLayer's own touch handling - registers/unregisters automatically
    // tied to onEnter/onExit, unlike a manual CCDirector dispatcher call.
    this->setTouchEnabled(true);
    this->setTouchMode(kCCTouchesOneByOne);
    this->setTouchPriority(-1000);

    return true;
}

bool FloatingButton::ccTouchBegan(CCTouch* touch, CCEvent*) {
    auto local = this->convertToNodeSpace(touch->getLocation());
    CCRect box(0, 0, this->getContentSize().width, this->getContentSize().height);
    if (!box.containsPoint(local)) return false;

    m_pressPos = touch->getLocation();
    m_dragging = true;
    m_touchMoved = false;
    return true;
}

void FloatingButton::ccTouchMoved(CCTouch* touch, CCEvent*) {
    if (!m_dragging) return;

    auto loc = touch->getLocation();
    auto delta = loc - m_pressPos;
    if (std::abs(delta.x) > 4.f || std::abs(delta.y) > 4.f) {
        m_touchMoved = true;
    }

    this->setPosition(this->getPosition() + delta);
    m_pressPos = loc;

    // Keep the button on screen.
    auto winSize = CCDirector::sharedDirector()->getWinSize();
    auto pos = this->getPosition();
    pos.x = std::clamp(pos.x, 25.f, winSize.width - 25.f);
    pos.y = std::clamp(pos.y, 25.f, winSize.height - 25.f);
    this->setPosition(pos);
}

void FloatingButton::ccTouchEnded(CCTouch*, CCEvent*) {
    if (m_dragging && !m_touchMoved) {
        this->onTap();
    }
    m_dragging = false;
}

void FloatingButton::onTap() {
    ModMenuPopup::create()->show();
}

void FloatingButton::onEnter() {
    CCLayer::onEnter();
    // Some layers add extra decorations/overlays right after their own
    // init() runs, which could end up rendering on top of (hiding) this
    // button even though it was added with a very high zOrder initially.
    // Re-assert front-most a frame later as a safety net.
    this->scheduleOnce(schedule_selector(FloatingButton::bringToFront), 0.f);
}

void FloatingButton::bringToFront(float) {
    if (auto parent = this->getParent()) {
        parent->reorderChild(this, 100000);
    }
}
