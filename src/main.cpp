#include <M5Unified.h>
#include <Avatar.h>
#include <unordered_map>
#include "BLEDevice.h"
#include "BLEServer.h"
#include <BLE2902.h>

#define SERVICE_UUID "0B21C05A-44C2-47CC-BFEF-4F7165C33908"
#define CHARACTERISTIC_UUID "B3C450C9-5FC5-48F6-9EFD-D588E494F462"

using namespace m5avatar;

bool connected = false;

class MyServerCallbacks : public BLEServerCallbacks
{
    void onConnect(BLEServer *pServer)
    {
        connected = true;
    };

    void onDisconnect(BLEServer *pServer)
    {
        connected = false;
    }
};

int expression = static_cast<int>(Expression::Neutral);
bool isExpressionChanged = false;

class ExpressionCharacteristicCallbacks : public BLECharacteristicCallbacks
{
    void onWrite(BLECharacteristic *pCharacteristic)
    {        
        std::string value = pCharacteristic->getValue();
        if (value.length() > 0)
        {
            if (value[0] == expression)
            {
                return;
            }
            expression = value[0];
            isExpressionChanged = true;
        }
    }
};

void setupServer()
{
    BLEDevice::init("M5AtomS3");
    BLEServer *pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    BLEService *pService = pServer->createService(SERVICE_UUID);
    BLECharacteristic *pExpressionCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ |
            BLECharacteristic::PROPERTY_WRITE);
    pExpressionCharacteristic->addDescriptor(new BLE2902());
    pExpressionCharacteristic->setCallbacks(new ExpressionCharacteristicCallbacks());

    pService->start();

    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
}

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

    void onExit() override
    {
        setupAvatar();
        setupServer();
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
        if (connected)
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
public:
    void onUpdate() override
    {
        if (isExpressionChanged)
        {
            avatar.setExpression(static_cast<Expression>(expression));
            isExpressionChanged = false;
        }
    }
};

void setup()
{
    auto cfg = M5.config();
    M5.begin(cfg);
    USBSerial.begin(115200);
    currentState = State::Start;
    nextState = State::Start;
    StartStateBehavior *startStateBehavior = new StartStateBehavior();
    stateBehavior = startStateBehavior;
    stateBehaviors[State::Start] = startStateBehavior;
    stateBehaviors[State::Ready] = new ReadyStateBehavior();
    stateBehaviors[State::Idle] = new IdleStateBehavior();
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