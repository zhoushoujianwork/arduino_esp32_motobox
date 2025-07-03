#include "SDManager.h"

SDManager::SDManager() : _initialized(false) {}

SDManager::~SDManager() {
    if (_initialized) {
        end();
    }
}

bool SDManager::begin() {
    if (_initialized) {
        return true;
    }

    debugPrint("初始化SD卡...");

#ifdef SD_MODE_SPI
    // SPI模式初始化
    SPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
    if (!SD.begin(SD_CS_PIN)) {
        debugPrint("❌ SD卡SPI模式初始化失败");
        return false;
    }
#else
    // MMC模式初始化
    if (!SD_MMC.begin()) {
        debugPrint("❌ SD卡MMC模式初始化失败");
        return false;
    }
#endif

    _initialized = true;
    debugPrint("✅ SD卡初始化成功");

    // 创建必要的目录结构
    createDirectoryStructure();
    
    // 保存设备信息
    saveDeviceInfo();

    return true;
}

void SDManager::end() {
    if (!_initialized) {
        return;
    }

#ifdef SD_MODE_SPI
    SD.end();
#else
    SD_MMC.end();
#endif

    _initialized = false;
    debugPrint("SD卡已断开");
}

bool SDManager::isInitialized() {
    return _initialized;
}

uint64_t SDManager::getTotalSpaceMB() {
    if (!_initialized) {
        return 0;
    }

#ifdef SD_MODE_SPI
    return SD.totalBytes() / (1024 * 1024);
#else
    return SD_MMC.totalBytes() / (1024 * 1024);
#endif
}

uint64_t SDManager::getFreeSpaceMB() {
    if (!_initialized) {
        return 0;
    }

#ifdef SD_MODE_SPI
    return (SD.totalBytes() - SD.usedBytes()) / (1024 * 1024);
#else
    return (SD_MMC.totalBytes() - SD_MMC.usedBytes()) / (1024 * 1024);
#endif
}

bool SDManager::createDirectoryStructure() {
    if (!_initialized) {
        return false;
    }

    // 创建基本目录结构
    const char* directories[] = {
        "/data",
        "/data/gps",
        "/config"
    };

    for (int i = 0; i < 3; i++) {
        if (!createDirectory(directories[i])) {
            debugPrint("创建目录失败: " + String(directories[i]));
            return false;
        }
    }

    debugPrint("✅ 目录结构创建完成");
    return true;
}

bool SDManager::createDirectory(const char* path) {
    if (!_initialized) {
        return false;
    }

#ifdef SD_MODE_SPI
    File dir = SD.open(path);
    if (!dir) {
        return SD.mkdir(path);
    }
#else
    File dir = SD_MMC.open(path);
    if (!dir) {
        return SD_MMC.mkdir(path);
    }
#endif

    dir.close();
    return true;
}

bool SDManager::saveDeviceInfo() {
    if (!_initialized) {
        return false;
    }

    const char* filename = "/config/device_info.json";
    
#ifdef SD_MODE_SPI
    File file = SD.open(filename, FILE_WRITE);
#else
    File file = SD_MMC.open(filename, FILE_WRITE);
#endif

    if (!file) {
        debugPrint("无法创建设备信息文件");
        return false;
    }

    // 创建设备信息JSON
    String deviceInfo = "{\n";
    deviceInfo += "  \"device_id\": \"" + getDeviceID() + "\",\n";
    deviceInfo += "  \"firmware_version\": \"" + String(FIRMWARE_VERSION) + "\",\n";
    deviceInfo += "  \"created_at\": \"" + getCurrentTimestamp() + "\",\n";
    deviceInfo += "  \"sd_total_mb\": " + String((unsigned long)getTotalSpaceMB()) + ",\n";
    deviceInfo += "  \"sd_free_mb\": " + String((unsigned long)getFreeSpaceMB()) + "\n";
    deviceInfo += "}";

    file.print(deviceInfo);
    file.close();

    debugPrint("✅ 设备信息已保存");
    return true;
}

bool SDManager::recordGPSData(double latitude, double longitude, double altitude, float speed, int satellites) {
    if (!_initialized) {
        return false;
    }

    // 生成文件名（按日期）
    String filename = "/data/gps/gps_" + getCurrentDateString() + ".geojson";
    
    // 检查文件是否存在，如果不存在则创建GeoJSON头部
    bool fileExists = false;
#ifdef SD_MODE_SPI
    File testFile = SD.open(filename.c_str(), FILE_READ);
#else
    File testFile = SD_MMC.open(filename.c_str(), FILE_READ);
#endif
    
    if (testFile) {
        fileExists = true;
        testFile.close();
    }

    // 打开文件进行写入
#ifdef SD_MODE_SPI
    File file = SD.open(filename.c_str(), FILE_APPEND);
#else
    File file = SD_MMC.open(filename.c_str(), FILE_APPEND);
#endif

    if (!file) {
        debugPrint("无法打开GPS数据文件: " + filename);
        return false;
    }

    // 如果是新文件，写入GeoJSON头部
    if (!fileExists) {
        file.println("{");
        file.println("  \"type\": \"FeatureCollection\",");
        file.println("  \"features\": [");
    } else {
        // 如果文件已存在，需要在最后一个特征后添加逗号
        file.print(",\n");
    }

    // 写入GPS数据点
    String gpsFeature = "    {\n";
    gpsFeature += "      \"type\": \"Feature\",\n";
    gpsFeature += "      \"geometry\": {\n";
    gpsFeature += "        \"type\": \"Point\",\n";
    gpsFeature += "        \"coordinates\": [" + String(longitude, 6) + ", " + String(latitude, 6) + ", " + String(altitude, 2) + "]\n";
    gpsFeature += "      },\n";
    gpsFeature += "      \"properties\": {\n";
    gpsFeature += "        \"timestamp\": \"" + getCurrentTimestamp() + "\",\n";
    gpsFeature += "        \"speed\": " + String(speed, 2) + ",\n";
    gpsFeature += "        \"satellites\": " + String(satellites) + "\n";
    gpsFeature += "      }\n";
    gpsFeature += "    }";

    file.print(gpsFeature);
    file.close();

    debugPrint("📍 GPS数据已记录: " + String(latitude, 6) + "," + String(longitude, 6));
    return true;
}

String SDManager::getDeviceID() {
    // 使用ESP32的MAC地址作为设备ID
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    
    String deviceId = "";
    for (int i = 0; i < 6; i++) {
        if (mac[i] < 16) deviceId += "0";
        deviceId += String(mac[i], HEX);
        if (i < 5) deviceId += ":";
    }
    deviceId.toUpperCase();
    return deviceId;
}

String SDManager::getCurrentTimestamp() {
    // 简单的时间戳，实际项目中应该使用RTC或NTP时间
    return String(millis());
}

String SDManager::getCurrentDateString() {
    // 简单的日期字符串，实际项目中应该使用真实日期
    unsigned long days = millis() / (24 * 60 * 60 * 1000);
    return String(days);
}

void SDManager::debugPrint(const String& message) {
    Serial.println("[SDManager] " + message);
}

// 串口命令处理
bool SDManager::handleSerialCommand(const String& command) {
    if (!_initialized) {
        Serial.println("SD卡未初始化");
        return false;
    }

    if (command == "sd.info") {
        Serial.println("=== SD卡信息 ===");
        Serial.println("设备ID: " + getDeviceID());
        Serial.println("总容量: " + String((unsigned long)getTotalSpaceMB()) + " MB");
        Serial.println("剩余空间: " + String((unsigned long)getFreeSpaceMB()) + " MB");
        Serial.println("初始化状态: " + String(_initialized ? "已初始化" : "未初始化"));
        return true;
    }
    else if (command == "sd.test") {
        // 测试GPS数据记录
        Serial.println("测试GPS数据记录...");
        bool result = recordGPSData(39.9042, 116.4074, 50.0, 25.5, 8);
        Serial.println("测试结果: " + String(result ? "成功" : "失败"));
        return result;
    }
    else if (command == "yes_format") {
        // 简化版暂不支持格式化功能
        Serial.println("简化版SD管理器暂不支持格式化功能");
        return false;
    }
    
    Serial.println("未知命令: " + command);
    return false;
}
