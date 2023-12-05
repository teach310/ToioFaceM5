#include <M5Unified.h>
#include "BLEDevice.h"
#include <BLE2902.h>

class BLE;

class BLE
{
public:
    void start();
    bool isConnected();
    int getExpression();
    bool isExpressionChanged();
    void sendDistance(uint16_t distance);

private:
    BLECharacteristic *pDistanceCharacteristic;
    std::string distanceNotifyData(uint16_t value);
};
