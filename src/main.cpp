#include <M5Unified.h>
#include "Adafruit_VL53L0X.h"
#include <Avatar.h>
#include <unordered_map>
#include "BLE.h"

using namespace m5avatar;

enum class State
{
    Start,
    Ready,
    Idle,
};

class BaseStateBehavior;

BLE ble = BLE();
Adafruit_VL53L0X lox = Adafruit_VL53L0X();
Avatar avatar;
State currentState = State::Start;
State nextState = State::Start;
BaseStateBehavior *pStateBehavior;
std::unordered_map<State, BaseStateBehavior *> stateBehaviors;

void setupLox()
{
    Wire.setPins(SDA, SCL);
    if (!lox.begin())
    {
        M5.Display.println(F("Failed to boot VL53L0X"));
        while (1)
            ;
    }
}

void setupAvatar()
{
    avatar.setScale(0.4);
    avatar.setPosition(-56, -96);

    // 目と口のサイズを1.5倍にする
    Face *pFace = avatar.getFace();
    pFace->setLeftEye(new Eye(12, false));
    pFace->setRightEye(new Eye(12, true));
    pFace->setMouth(new Mouth(50, 90, 6, 60));
    avatar.init();
}

class BaseStateBehavior
{
public:
    virtual void onEnter() {}
    virtual void onUpdate() {}
    virtual void onExit() {}
};

void setNextState(State state)
{
    nextState = state;
}

void changeState()
{
    if (pStateBehavior != nullptr)
    {
        pStateBehavior->onExit();
    }
    currentState = nextState;
    pStateBehavior = stateBehaviors[currentState];
    pStateBehavior->onEnter();
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

    void onExit() override
    {
        setupAvatar();
        ble.start();
        setupLox();
    }
};

class ReadyStateBehavior : public BaseStateBehavior
{
public:
    void onEnter() override
    {
        BLEDevice::startAdvertising();
    }

    void onUpdate() override
    {
        if (ble.isConnected())
        {
            setNextState(State::Idle);
        }
    }

    void onExit() override
    {
        BLEDevice::stopAdvertising();
    }
};

class IdleStateBehavior : public BaseStateBehavior
{
private:
    unsigned long latestReadRangeTime = 0;
    const unsigned long readRangeInterval = 100;

public:
    void onEnter() override
    {
        lox.startRangeContinuous();
    }

    void onUpdate() override
    {
        if (!ble.isConnected())
        {
            setNextState(State::Ready);
            return;
        }

        if (ble.isExpressionChanged())
        {
            avatar.setExpression(static_cast<Expression>(ble.getExpression()));
        }

        unsigned long now = millis();
        if (now - latestReadRangeTime >= readRangeInterval && lox.isRangeComplete())
        {
            uint16_t distance = lox.readRange();
            ble.sendDistance(distance);
            latestReadRangeTime = now;
        }
    }

    void onExit() override
    {
        lox.stopRangeContinuous();
    }
};

void registerBehaviors()
{
    stateBehaviors[State::Start] = new StartStateBehavior();
    stateBehaviors[State::Ready] = new ReadyStateBehavior();
    stateBehaviors[State::Idle] = new IdleStateBehavior();
}

void setup()
{
    auto cfg = M5.config();
    M5.begin(cfg);
    USBSerial.begin(115200);
    registerBehaviors();
    pStateBehavior = stateBehaviors[State::Start];
    pStateBehavior->onEnter();
}

void loop()
{
    if (currentState != nextState)
    {
        changeState();
    }
    pStateBehavior->onUpdate();
}
