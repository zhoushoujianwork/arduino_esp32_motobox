#include "ble_server.h"

class ServerCallbacks : public NimBLEServerCallbacks
{
    void onConnect(NimBLEServer *pServer)
    {
        Serial.println("客户端已连接");
    };

    void onDisconnect(NimBLEServer *pServer)
    {
        Serial.println("客户端已断开连接 - 重新开始广播");
        // NimBLEDevice::startAdvertising();
    };
};

class GpsCharacteristicCallbacks : public NimBLECharacteristicCallbacks
{
    void onRead(NimBLECharacteristic *pGPSCharacteristic)
    {
#if Enable_GPS
        // Serial.println("Enable_GPS");
        pGPSCharacteristic->setValue((uint8_t *)gps.get_gps_data(), sizeof(gps_data_t));
#endif
    }
};

class ImuCharacteristicCallbacks : public NimBLECharacteristicCallbacks
{
    void onRead(NimBLECharacteristic *pIMUCharacteristic)
    {
#if Enable_IMU
        // Serial.println("Enable_IMU");
        pIMUCharacteristic->setValue((uint8_t *)imu.get_imu_data(), sizeof(imu_data_t));
#endif
    }
};

BLES::BLES()
{
    pServer = NULL;
    pCharacteristic = NULL;
    pGPSCharacteristic = NULL;
    pIMUCharacteristic = NULL;
}

void BLES::setup()
{
    Serial.println("初始化BLE服务器...");

    // 初始化BLE设备
    NimBLEDevice::init(BLE_NAME);

    // 设置发射功率
    NimBLEDevice::setPower(ESP_PWR_LVL_P3); // +3dB

    // 创建BLE服务器
    pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());

    // 创建BLE服务
    NimBLEService *pService = pServer->createService(SERVICE_UUID);

    // 创建设备状态特征值
    pCharacteristic = pService->createCharacteristic(
        DEVICE_CHAR_UUID,
        NIMBLE_PROPERTY::NOTIFY); // 根据需要设置属性

    // 创建GPS特征值
    pGPSCharacteristic = pService->createCharacteristic(
        GPS_CHAR_UUID,
        NIMBLE_PROPERTY::READ);

    pGPSCharacteristic->setCallbacks(new GpsCharacteristicCallbacks());

    // 创建IMU特征值
    pIMUCharacteristic = pService->createCharacteristic(
        IMU_CHAR_UUID,
        NIMBLE_PROPERTY::READ);

    pIMUCharacteristic->setCallbacks(new ImuCharacteristicCallbacks());

    // 启动服务
    pService->start();

    // 开始广播
    NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->start();

    Serial.println("BLE服务器已启动");
}

void BLES::loop()
{
    if (pServer == nullptr)
    {
        Serial.println("BLE服务器未初始化");
        return;
    }

    if (pServer->getConnectedCount() != 0)
    {
        device.set_ble_connected(true);

        if (pCharacteristic == nullptr)
        {
            Serial.println("pCharacteristic 未初始化");
            return;
        }

        device_state_t *pdevice = device.get_device_state();
        if (pdevice == nullptr)
        {
            Serial.println("设备状态指针无效");
            return;
        }

        // 打印调试信息k
        // Serial.printf("设备状态大小: %d\n", sizeof(device_state_t));

        // 使用临时变量存储状态
        device_state_t temp_state = *pdevice;
        pCharacteristic->setValue((uint8_t *)&temp_state, sizeof(device_state_t));
        pCharacteristic->notify();
    }
    else
    {
        device.set_ble_connected(false);
    }
}
