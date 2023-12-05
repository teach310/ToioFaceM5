
#include "BLE.h"

#define SERVICE_UUID "0B21C05A-44C2-47CC-BFEF-4F7165C33908"
#define EXPRESSION_CHARACTERISTIC_UUID "B3C450C9-5FC5-48F6-9EFD-D588E494F462"
#define DISTANCE_CHARACTERISTIC_UUID "29C2D1B2-944A-4FBA-AFCD-133E09532556"

static bool connected = false;
static int expression = -1;
static bool isExpressionChanged = false;

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

void BLE::start()
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

int BLE::getExpression()
{
    return expression;
}

bool BLE::isExpressionChanged()
{
    return isExpressionChanged;
}

bool BLE::isConnected()
{
    return connected;
}

std::string BLE::distanceNotifyData(uint16_t value)
{
    std::string data;
    data += static_cast<char>(value & 0xff);
    data += static_cast<char>((value >> 8) & 0xff);
    return data;
}

void BLE::sendDistance(uint16_t distance)
{
    pDistanceCharacteristic->setValue(distanceNotifyData(distance));
    pDistanceCharacteristic->notify();
}
