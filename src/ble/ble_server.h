#ifndef BLE_SERVER_H
#define BLE_SERVER_H

#include <NimBLEDevice.h>
#include "device.h"
#include "config.h"
#include "imu/qmi8658.h"
#include "power/PowerManager.h"
#include "wifi/server.h"

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
    void begin();
    void loop();
    bool isConnected() const { return connected; }
    void startScan();
    static void handleScanResults(NimBLEAdvertisedDevice* advertisedDevice);
    static TirePressureData lastTirePressureData;

private:
    NimBLEServer *pServer;
    NimBLECharacteristic *pCharacteristic;
    NimBLECharacteristic *pGPSCharacteristic;
    NimBLECharacteristic *pIMUCharacteristic;

    bool connected;

    unsigned long lastBlePublishTime = 0;
    
    static bool isValidTirePressureData(NimBLEAdvertisedDevice* advertisedDevice);
    static TirePressureData parseTirePressureData(uint8_t* data, size_t length);
};

#ifdef BLE_SERVER
extern BLES bs;
#endif

#endif