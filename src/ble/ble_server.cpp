#include "ble_server.h"

// 初始化静态成员
TirePressureData BLES::lastTirePressureData = {0};

class ServerCallbacks : public NimBLEServerCallbacks
{
    void onConnect(NimBLEServer *pServer)
    {
        Serial.println("客户端已连接");
        // 连接后停止扫描以节省资源
        NimBLEDevice::getScan()->stop();
    };

    void onDisconnect(NimBLEServer *pServer)
    {
        Serial.println("客户端已断开连接 - 重新开始广播");
        // 断开连接后恢复扫描
        NimBLEDevice::getScan()->start(0, nullptr, false);
    };
};

class GpsCharacteristicCallbacks : public NimBLECharacteristicCallbacks
{
    void onRead(NimBLECharacteristic *pGPSCharacteristic)
    {
        pGPSCharacteristic->setValue((uint8_t *)device.get_gps_data(), sizeof(gps_data_t));
    }
};

class ImuCharacteristicCallbacks : public NimBLECharacteristicCallbacks
{
    void onRead(NimBLECharacteristic *pIMUCharacteristic)
    {
        pIMUCharacteristic->setValue((uint8_t *)device.get_imu_data(), sizeof(imu_data_t));
    }
};

class ScanCallbacks : public NimBLEAdvertisedDeviceCallbacks {
    void onResult(NimBLEAdvertisedDevice* advertisedDevice) {
        BLES::handleScanResults(advertisedDevice);
    }
};

// 用于设备状态特征的回调，支持写入命令
class DeviceCharacteristicCallbacks : public NimBLECharacteristicCallbacks
{
    void onWrite(NimBLECharacteristic *pCharacteristic)
    {
        // 处理 BLE 写入命令
        std::string value = pCharacteristic->getValue();
        if (!value.empty()) {
            switch (value[0]) {
                case 0x01:
                    Serial.println("收到 BLE 重置 WiFi 命令");
                    wifiManager.reset();
                    break;
                case 0x02:
                    Serial.println("收到 BLE 进入配网模式命令");
                    wifiManager.enterConfigMode();
                    break;
                case 0x03:
                    Serial.println("收到 BLE 退出配网模式命令");
                    wifiManager.exitConfigMode();
                    break;
                // case 0x04:
                //     Serial.println("收到 BLE 进入睡眠模式");
                //     powerManager.requestLowPowerMode = true;
                //     powerManager.enterLowPowerMode();
                //     break;
                default:
                    Serial.printf("收到未知 BLE 命令: 0x%02X\n", value[0]);
                    break;
            }
        }
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

    // 设置发射功率 - 降低功率以减少干扰
    NimBLEDevice::setPower(ESP_PWR_LVL_N0); // 0dB

    // 创建BLE服务器
    pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());

    // 创建BLE服务
    NimBLEService *pService = pServer->createService(SERVICE_UUID);

    // 创建设备状态特征值
    pCharacteristic = pService->createCharacteristic(
        DEVICE_CHAR_UUID,
        NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::WRITE); // 增加 WRITE 属性
    pCharacteristic->setCallbacks(new DeviceCharacteristicCallbacks());

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

    // 配置广播参数
    NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    // 设置广播间隔 (单位: 0.625ms)
    pAdvertising->setMinInterval(32); // 20ms
    pAdvertising->setMaxInterval(64); // 40ms
    pAdvertising->start();

    // 等待广播稳定
    delay(500);

    // debug 先关掉Scan
    return;

    // 服务器完全初始化后再启动扫描
    startScan();

    Serial.println("BLE服务器已启动");
}

void BLES::startScan() {
    Serial.println("启动BLE扫描...");
    try {
        // 获取扫描对象
        NimBLEScan* pScan = NimBLEDevice::getScan();
        
        // 停止之前的扫描
        pScan->stop();
        delay(100);  // 等待扫描完全停止
        
        // 设置扫描回调
        pScan->setAdvertisedDeviceCallbacks(new ScanCallbacks(), false);
        
        // 设置被动扫描，减少对广播的影响
        pScan->setActiveScan(false);
        // 设置较长的扫描间隔和较短的扫描窗口
        pScan->setInterval(3200);  // 2秒间隔
        pScan->setWindow(400);     // 250ms扫描窗口
        
        // 开始持续扫描
        if(pScan->start(0, nullptr, false)) {
            Serial.println("BLE扫描已启动 - 被动扫描模式");
        } else {
            Serial.println("BLE扫描启动失败，将在下一个循环重试");
        }
    } catch (const std::exception& e) {
        Serial.printf("扫描启动异常: %s\n", e.what());
    }
}

bool BLES::isValidTirePressureData(NimBLEAdvertisedDevice* advertisedDevice) {
    // 打印设备地址和UUID信息
    Serial.printf("设备地址: %s\n", advertisedDevice->getAddress().toString().c_str());
    if(advertisedDevice->haveServiceUUID()) {
        NimBLEUUID serviceUUID = advertisedDevice->getServiceUUID();
        if(serviceUUID.bitSize() == 16) {
            // 如果是16位短UUID
            Serial.printf("服务UUID(16位): 0x%04X\n", serviceUUID.getNative()->u16.value);
        } else {
            // 如果是128位完整UUID
            Serial.printf("服务UUID(128位): %s\n", serviceUUID.toString().c_str());
        }
    }

    // 打印完整的广播数据
    Serial.print("完整广播数据: ");
    Serial.println(advertisedDevice->toString().c_str());

    // 如果设备有名称，打印名称
    if(advertisedDevice->haveName()) {
        Serial.printf("设备名称: %s\n", advertisedDevice->getName().c_str());
    }

    // 如果有TX Power，打印发射功率
    if(advertisedDevice->haveTXPower()) {
        Serial.printf("发射功率: %d\n", advertisedDevice->getTXPower());
    }

    uint8_t *payload = advertisedDevice->getPayload();
    size_t payloadLen = advertisedDevice->getPayloadLength();
    if(payload != nullptr && payloadLen > 0) {
        Serial.print("原始广播数据: ");
        for(int i = 0; i < payloadLen; i++) {
            Serial.printf("%02X ", payload[i]);
        }
        Serial.println();
    }

    // 设备 address 后续进行匹配绑定
    if (advertisedDevice->getAddress().toString() == "48:42:00:00:ab:73") {
        return true;
    }
    return false;
}

TirePressureData BLES::parseTirePressureData(uint8_t* data, size_t length) {
    TirePressureData result = {0};
    
    // 数据格式: 80 1F 13 01 B9 0F 8D
    // 0-1: 厂商ID (0x801F)
    // 2: 数据类型 (0x13)
    // 3-6: 实际数据
    
    // 打印完整数据格式用于调试
    Serial.print("解析数据: ");
    for(int i = 0; i < length; i++) {
        Serial.printf("%02X ", data[i]);
    }
    Serial.println();
    
    // 提取设备ID - 4字节组成32位ID
    result.deviceId = ((uint32_t)data[3] << 24) | 
                     ((uint32_t)data[4] << 16) | 
                     ((uint32_t)data[5] << 8) | 
                     data[6];
    
    // 胎压 - 根据实际测试值推导
    // 0x01 = 2.0 bar
    result.pressure = data[3];
    
    // 温度 - 根据实际测试值推导
    // 0xB9 (185) = 19°C
    result.temperature = data[4];
    
    // 电池电量 - 根据实际测试值推导
    // 0x0F (15) = 3.1V
    result.battery = data[5];
    
    return result;
}

/*
02 01 06 06 09 54 50 4d 53 53 14 FF 4d 61 00 00 00 00 94 8b 71 61 00 1C b0 b2 19 26 17 67 6d
物理连接功能域： 02 01 06 (hex) 
蓝牙名域 "TPMSS"： 06 09 54 50 4d 53 53(hex) 
自定义域： 
14 FF 4d 61 00 00 00 00 94 8b 71 61 00 1C b0 b2 19 26 17 67 6d (hex)

// 实际
03 03 A5 27 03 08 42 52 08 FF 28 1F 12 00 92 96 AC
*/
void BLES::handleScanResults(NimBLEAdvertisedDevice* advertisedDevice) {
    if (!isValidTirePressureData(advertisedDevice)) {
        return;
    }
    
    std::string data = advertisedDevice->getManufacturerData();
    TirePressureData tireData = parseTirePressureData((uint8_t*)data.data(), data.length());
    
    // 转换实际值 - 根据实测数据调整公式
    // 样本: 01 B9 0F 8D => 2.0 bar, 19°C, 3.1V
    float pressure = (float)tireData.pressure * 2.0f; // Bar (01 => 2.0 bar)
    float temperature = 204.0f - (float)tireData.temperature; // °C (B9 => 19°C)
    float battery = (float)tireData.battery * 0.2067f + 0.0f; // V (0F => 3.1V)
    
    Serial.printf("设备ID: 0x%08X\n", tireData.deviceId);
    Serial.printf("胎压: %.1f Bar\n", pressure);
    Serial.printf("温度: %.1f ℃\n", temperature);
    Serial.printf("电池: %.1f V\n", battery);
    Serial.println("------------------------");
    
    lastTirePressureData = tireData;
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
