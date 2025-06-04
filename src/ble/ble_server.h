#ifndef BLE_SERVER_H
#define BLE_SERVER_H

#include <NimBLEDevice.h>
#include "device.h"
#include "config.h"
#include "gps/GPS.h"
#include "qmi8658/IMU.h"
#include "wifi/WifiManager.h"

// 胎压数据结构
struct TirePressureData {
    uint8_t pressure;    // 胎压
    uint8_t temperature; // 温度
    uint8_t battery;     // 电池电量
    uint32_t deviceId;   // 设备ID
};

class BLES
{
public:
    BLES();
    void setup();
    void loop();
    void startScan();
    static void handleScanResults(NimBLEAdvertisedDevice* advertisedDevice);
    static TirePressureData lastTirePressureData;

private:
    NimBLEServer *pServer;
    NimBLECharacteristic *pCharacteristic;
    NimBLECharacteristic *pGPSCharacteristic;
    NimBLECharacteristic *pIMUCharacteristic;
    
    static bool isValidTirePressureData(NimBLEAdvertisedDevice* advertisedDevice);
    static TirePressureData parseTirePressureData(uint8_t* data, size_t length);
};

#endif