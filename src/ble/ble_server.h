#ifndef BLE_SERVER_H
#define BLE_SERVER_H

#include <NimBLEDevice.h>
#include "device.h"
#include "config.h"
#include "gps/GPS.h"
#include "qmi8658/IMU.h"

class BLES
{
public:
    BLES();
    void setup();
    void loop();

private:
    NimBLEServer *pServer;
    NimBLECharacteristic *pCharacteristic;
    NimBLECharacteristic *pGPSCharacteristic;
    NimBLECharacteristic *pIMUCharacteristic;
};

#endif