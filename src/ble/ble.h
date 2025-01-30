#ifndef BLE_H
#define BLE_H

#include <NimBLEDevice.h>
#include "device.h"
#include "config.h"
#include "gps/GPS.h"
#include "qmi8658/IMU.h"

class BLE
{
public:
    BLE();
    void begin();
    void loop();

private:
    NimBLEServer *pServer;
    NimBLECharacteristic *pCharacteristic;
    NimBLECharacteristic *pGPSCharacteristic;
    NimBLECharacteristic *pIMUCharacteristic;
};

#endif