#include <M5Unified.h>
#include <Avatar.h>
#include <unordered_map>

using namespace m5avatar;

Avatar avatar;

void setupAvatar()
{
    avatar.setScale(0.4);
    avatar.setPosition(-56, -96);

    // 目と口のサイズを1.5倍にする
    Face *face = avatar.getFace();
    face->setLeftEye(new Eye(12, false));
    face->setRightEye(new Eye(12, true));
    face->setMouth(new Mouth(50, 90, 6, 60));
    avatar.init();
}

enum class State
{
    Start,
    Ready,
    Idle
};

class BaseStateBehavior
{
public:
    virtual void onEnter() {}
    virtual void onUpdate() {}
    virtual void onExit() {}
};

State currentState;
State nextState;
BaseStateBehavior *stateBehavior;
std::unordered_map<State, BaseStateBehavior *> stateBehaviors;

void setNextState(State state)
{
    nextState = state;
}

void changeState()
{
    if (stateBehavior != nullptr)
    {
        stateBehavior->onExit();
    }
    currentState = nextState;
    stateBehavior = stateBehaviors[currentState];
    stateBehavior->onEnter();
}

class StartStateBehavior : public BaseStateBehavior
{
public:
    void onEnter() override
    {
        M5.Display.setTextSize(2);
        M5.Display.print("Press to start");
    }

    void onUpdate() override
    {
        M5.update();
        if (M5.BtnA.wasPressed())
        {
            setNextState(State::Ready);
        }
    }
};

class ReadyStateBehavior : public BaseStateBehavior
{
public:
    void onEnter() override
    {
        setupAvatar();
    }
};

class IdleStateBehavior : public BaseStateBehavior
{
public:
    void onEnter() override
    {
        avatar.setExpression(Expression::Happy);
    }
};

void setup()
{
    auto cfg = M5.config();
    M5.begin(cfg);
    currentState = State::Start;
    nextState = State::Start;
    StartStateBehavior *startStateBehavior = new StartStateBehavior();
    stateBehavior = startStateBehavior;
    stateBehaviors[State::Start] = startStateBehavior;
    stateBehaviors[State::Ready] = new ReadyStateBehavior();
    stateBehaviors[State::Idle] = new BaseStateBehavior();
    stateBehavior->onEnter();
}

void loop()
{
    if (currentState != nextState)
    {
        changeState();
    }
    stateBehavior->onUpdate();
}