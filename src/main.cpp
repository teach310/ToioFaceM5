#include <M5Unified.h>
#include "Adafruit_VL53L0X.h"
#include <Avatar.h>
#include <unordered_map>
#include "BLEDevice.h"
#include "BLEServer.h"
#include <BLE2902.h>

#define SERVICE_UUID "0B21C05A-44C2-47CC-BFEF-4F7165C33908"
#define EXPRESSION_CHARACTERISTIC_UUID "B3C450C9-5FC5-48F6-9EFD-D588E494F462"
#define DISTANCE_CHARACTERISTIC_UUID "29C2D1B2-944A-4FBA-AFCD-133E09532556"

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

BLECharacteristic *pDistanceCharacteristic;

void setupServer()
{
    BLEDevice::init("M5AtomS3");
    BLEServer *pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    BLEService *pService = pServer->createService(SERVICE_UUID);
    BLECharacteristic *pExpressionCharacteristic = pService->createCharacteristic(
        EXPRESSION_CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ |
            BLECharacteristic::PROPERTY_WRITE);
    pExpressionCharacteristic->addDescriptor(new BLE2902());
    pExpressionCharacteristic->setCallbacks(new ExpressionCharacteristicCallbacks());

    pDistanceCharacteristic = pService->createCharacteristic(
        DISTANCE_CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ |
            BLECharacteristic::PROPERTY_NOTIFY);
    pDistanceCharacteristic->addDescriptor(new BLE2902());

    pService->start();

    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
}

Adafruit_VL53L0X lox = Adafruit_VL53L0X();

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

Avatar avatar;

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
BaseStateBehavior *pStateBehavior;
std::unordered_map<State, BaseStateBehavior *> stateBehaviors;

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
        setupServer();
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
private:
    unsigned long latestReadRangeTime = 0;
    const unsigned long readRangeInterval = 100;

    std::string distanceNotifyData(uint16_t value)
    {
        std::string data;
        data += static_cast<char>(value & 0xff);
        data += static_cast<char>((value >> 8) & 0xff);
        return data;
    }

    void readDistance()
    {
        uint16_t distance = lox.readRange();
        pDistanceCharacteristic->setValue(distanceNotifyData(distance));
        pDistanceCharacteristic->notify();
    }

public:
    void onEnter() override
    {
        lox.startRangeContinuous();
    }

    void onUpdate() override
    {
        if (isExpressionChanged)
        {
            avatar.setExpression(static_cast<Expression>(expression));
            isExpressionChanged = false;
        }

        unsigned long now = millis();
        if (now - latestReadRangeTime >= readRangeInterval && lox.isRangeComplete())
        {
            readDistance();
            latestReadRangeTime = now;
        }
    }

    void onExit() override
    {
        lox.stopRangeContinuous();
    }
};

void setup()
{
    auto cfg = M5.config();
    M5.begin(cfg);
    USBSerial.begin(115200);
    currentState = State::Start;
    nextState = State::Start;
    StartStateBehavior *pStartStateBehavior = new StartStateBehavior();
    pStateBehavior = pStartStateBehavior;
    stateBehaviors[State::Start] = pStartStateBehavior;
    stateBehaviors[State::Ready] = new ReadyStateBehavior();
    stateBehaviors[State::Idle] = new IdleStateBehavior();
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