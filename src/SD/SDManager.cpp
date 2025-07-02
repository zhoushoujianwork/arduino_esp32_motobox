/*
 * SD卡管理类实现 - 支持SPI和SDIO模式
 */

#include "SDManager.h"
#include "../device.h"
#include <Update.h>

#ifdef ENABLE_SDCARD

SDManager sdManager;

SDManager::SDManager() : _initialized(false), _debug(false), _lowPowerMode(false), _optimalSPIFrequency(4000000) {
}

bool SDManager::begin() {
    if (_initialized) {
        debugPrint("SD卡已经初始化");
        return true;
    }
    
    debugPrint("开始初始化SD卡...");
    
#ifdef SD_MODE_SPI
    // SPI模式初始化
    debugPrint("使用SPI模式初始化SD卡");
    debugPrint(String("SPI引脚配置: CS=") + SD_CS_PIN + ", MOSI=" + SD_MOSI_PIN + 
               ", MISO=" + SD_MISO_PIN + ", SCK=" + SD_SCK_PIN);
    
    // 检查引脚配置的有效性
    if (!validateSPIPins()) {
        debugPrint("❌ SPI引脚配置无效");
        return false;
    }
    
    // 先结束之前的SPI连接（如果有）
    SD.end();
    delay(100);
    
    // 初始化SPI总线，使用较低的频率开始
    SPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
    
    // 设置CS引脚为输出并拉高
    pinMode(SD_CS_PIN, OUTPUT);
    digitalWrite(SD_CS_PIN, HIGH);
    delay(10);
    
    // 尝试多次初始化，使用不同的频率
    const uint32_t frequencies[] = {400000, 1000000, 4000000}; // 从低频开始
    const char* freqNames[] = {"400kHz", "1MHz", "4MHz"};
    
    for (int i = 0; i < 3; i++) {
        debugPrint(String("尝试使用 ") + freqNames[i] + " 频率初始化SD卡");
        
        // 重置SPI设置
        SPI.end();
        delay(100);
        SPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
        
        // 设置CS引脚
        pinMode(SD_CS_PIN, OUTPUT);
        digitalWrite(SD_CS_PIN, HIGH);
        delay(10);
        
        // 尝试初始化SD卡
        if (SD.begin(SD_CS_PIN, SPI, frequencies[i])) {
            debugPrint(String("✅ SPI模式SD卡初始化成功 (") + freqNames[i] + ")");
            _optimalSPIFrequency = frequencies[i];
            break;
        } else {
            debugPrint(String("❌ ") + freqNames[i] + " 频率初始化失败");
            if (i == 2) {
                debugPrint("❌ 所有频率尝试失败，SD卡初始化失败");
                performHardwareDiagnostic();
                return false;
            }
        }
    }
    
#else
    // SDIO模式初始化
    debugPrint("使用SDIO模式初始化SD卡");
    
    // 先结束之前的连接
    SD_MMC.end();
    delay(100);
    
    // 尝试4位SDIO模式
    if (!SD_MMC.begin("/sdcard", false, true, SDMMC_FREQ_DEFAULT)) {
        debugPrint("4位SDIO模式失败，尝试1位模式");
        
        // 尝试1位模式
        if (!SD_MMC.begin("/sdcard", true, false, SDMMC_FREQ_DEFAULT)) {
            debugPrint("❌ SDIO模式SD卡初始化失败");
            performHardwareDiagnostic();
            return false;
        }
        debugPrint("✅ 1位SDIO模式初始化成功");
    } else {
        debugPrint("✅ 4位SDIO模式初始化成功");
    }
#endif
    
    _initialized = true;
    
    // 验证SD卡是否真正可用
    if (!verifySDCardOperation()) {
        debugPrint("❌ SD卡操作验证失败");
        _initialized = false;
        return false;
    }
    
    if (_debug) {
        printCardInfo();
    }
    
    // 检查是否需要初始化新卡
    bool isNewCard = !fileExists("/device_info.json");
    if (isNewCard) {
        debugPrint("检测到新SD卡，开始初始化...");
        if (!initializeNewCard()) {
            debugPrint("⚠️ 新卡初始化失败，但SD卡仍可使用");
            // 非致命错误，继续执行
        }
    } else {
        // 创建必要的目录（如果不存在）
        createDir("/config");
        createDir("/logs");
        createDir("/firmware");
        createDir("/data");
        createDir("/data/gps");
        createDir("/data/imu");
    }
    
    debugPrint("✅ SD卡初始化完成");
    return true;
}
void SDManager::end() {
    if (_initialized) {
#ifdef SD_MODE_SPI
        SD.end();
#else
        SD_MMC.end();
#endif
        _initialized = false;
        debugPrint("SD卡已断开");
    }
}

void SDManager::printCardInfo() {
    if (!_initialized) {
        debugPrint("SD卡未初始化");
        return;
    }
    
#ifdef SD_MODE_SPI
    uint8_t cardType = SD.cardType();
    String cardTypeStr;
    
    switch (cardType) {
        case CARD_MMC:
            cardTypeStr = "MMC";
            break;
        case CARD_SD:
            cardTypeStr = "SDSC";
            break;
        case CARD_SDHC:
            cardTypeStr = "SDHC";
            break;
        default:
            cardTypeStr = "UNKNOWN";
            break;
    }
    
    Serial.printf("SD卡类型: %s (SPI模式)\n", cardTypeStr.c_str());
    Serial.printf("SD卡大小: %lluMB\n", SD.cardSize() / (1024 * 1024));
    Serial.printf("剩余空间: %lluMB\n", (SD.totalBytes() - SD.usedBytes()) / (1024 * 1024));
    Serial.printf("最佳SPI频率: %u Hz\n", _optimalSPIFrequency);
    
#else
    uint8_t cardType = SD_MMC.cardType();
    String cardTypeStr;
    
    switch (cardType) {
        case CARD_MMC:
            cardTypeStr = "MMC";
            break;
        case CARD_SD:
            cardTypeStr = "SDSC";
            break;
        case CARD_SDHC:
            cardTypeStr = "SDHC";
            break;
        default:
            cardTypeStr = "UNKNOWN";
            break;
    }
    
    Serial.printf("SD卡类型: %s (SDIO模式)\n", cardTypeStr.c_str());
    Serial.printf("SD卡大小: %lluMB\n", getCardSizeMB());
    Serial.printf("剩余空间: %lluMB\n", getFreeSpaceMB());
#endif
}

uint64_t SDManager::getCardSizeMB() const {
    if (!_initialized) return 0;
#ifdef SD_MODE_SPI
    return SD.cardSize() / (1024 * 1024);
#else
    return SD_MMC.cardSize() / (1024 * 1024);
#endif
}

uint64_t SDManager::getFreeSpaceMB() const {
    if (!_initialized) return 0;
#ifdef SD_MODE_SPI
    return (SD.totalBytes() - SD.usedBytes()) / (1024 * 1024);
#else
    return (SD_MMC.totalBytes() - SD_MMC.usedBytes()) / (1024 * 1024);
#endif
}

// 基本文件操作实现
bool SDManager::writeFile(const char* filename, const char* content, bool append) {
    if (!_initialized) {
        debugPrint("SD卡未初始化，无法写入文件");
        return false;
    }
    
    File file;
#ifdef SD_MODE_SPI
    file = SD.open(filename, append ? FILE_APPEND : FILE_WRITE);
#else
    file = SD_MMC.open(filename, append ? FILE_APPEND : FILE_WRITE);
#endif

    if (!file) {
        debugPrint(String("无法打开文件进行写入: ") + filename);
        return false;
    }
    
    size_t bytesWritten = file.print(content);
    file.close();
    
    if (bytesWritten == 0) {
        debugPrint("写入文件失败: 0字节写入");
        return false;
    }
    
    debugPrint(String("文件写入成功: ") + filename + ", " + bytesWritten + " 字节");
    return true;
}

String SDManager::readFile(const char* filename) {
    if (!_initialized) {
        debugPrint("SD卡未初始化，无法读取文件");
        return "";
    }
    
    File file;
#ifdef SD_MODE_SPI
    file = SD.open(filename, FILE_READ);
#else
    file = SD_MMC.open(filename, FILE_READ);
#endif

    if (!file || file.isDirectory()) {
        debugPrint(String("无法打开文件进行读取: ") + filename);
        return "";
    }
    
    String content = "";
    while (file.available()) {
        content += (char)file.read();
    }
    file.close();
    
    debugPrint(String("文件读取成功: ") + filename + ", " + content.length() + " 字节");
    return content;
}

bool SDManager::fileExists(const char* filename) {
    if (!_initialized) return false;
    
#ifdef SD_MODE_SPI
    return SD.exists(filename);
#else
    return SD_MMC.exists(filename);
#endif
}

bool SDManager::deleteFile(const char* filename) {
    if (!_initialized) return false;
    
    if (!fileExists(filename)) {
        debugPrint(String("文件不存在，无法删除: ") + filename);
        return false;
    }
    
#ifdef SD_MODE_SPI
    bool success = SD.remove(filename);
#else
    bool success = SD_MMC.remove(filename);
#endif

    if (success) {
        debugPrint(String("文件删除成功: ") + filename);
    } else {
        debugPrint(String("文件删除失败: ") + filename);
    }
    
    return success;
}

bool SDManager::createDir(const char* dirname) {
    if (!_initialized) return false;
    
#ifdef SD_MODE_SPI
    bool success = SD.mkdir(dirname);
#else
    bool success = SD_MMC.mkdir(dirname);
#endif

    if (success) {
        debugPrint(String("目录创建成功: ") + dirname);
    } else {
        debugPrint(String("目录创建失败: ") + dirname);
    }
    
    return success;
}

void SDManager::listDir(const char* dirname) {
    if (!_initialized) {
        debugPrint("SD卡未初始化，无法列出目录");
        return;
    }
    
    File root;
#ifdef SD_MODE_SPI
    root = SD.open(dirname);
#else
    root = SD_MMC.open(dirname);
#endif

    if (!root) {
        debugPrint(String("无法打开目录: ") + dirname);
        return;
    }
    
    if (!root.isDirectory()) {
        debugPrint(String("不是目录: ") + dirname);
        return;
    }
    
    Serial.printf("目录列表: %s\n", dirname);
    
    File file = root.openNextFile();
    while (file) {
        if (file.isDirectory()) {
            Serial.printf("  DIR: %s\n", file.name());
        } else {
            Serial.printf("  FILE: %s (%u bytes)\n", file.name(), file.size());
        }
        file = root.openNextFile();
    }
    
    root.close();
}
// 配置文件管理
bool SDManager::saveConfig(const String& config, const char* filename) {
    return writeFile(filename, config.c_str());
}

String SDManager::loadConfig(const char* filename) {
    return readFile(filename);
}

bool SDManager::saveWiFiConfig(const String& ssid, const String& password) {
    StaticJsonDocument<256> doc;
    doc["ssid"] = ssid;
    doc["password"] = password;
    
    String jsonString;
    serializeJson(doc, jsonString);
    
    return saveConfig(jsonString, "/config/wifi.json");
}

bool SDManager::loadWiFiConfig(String& ssid, String& password) {
    String config = loadConfig("/config/wifi.json");
    if (config.isEmpty()) {
        return false;
    }
    
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, config);
    
    if (error) {
        debugPrint("WiFi配置JSON解析失败");
        return false;
    }
    
    ssid = doc["ssid"].as<String>();
    password = doc["password"].as<String>();
    
    return true;
}

// 日志管理
bool SDManager::writeLog(const String& message, const char* filename) {
    String logEntry = getTimestamp() + " " + message + "\n";
    return writeFile(filename, logEntry.c_str(), true); // 追加模式
}

String SDManager::readLog(const char* filename, int maxLines) {
    return readFile(filename);
}

bool SDManager::clearLog(const char* filename) {
    return writeFile(filename, "", false); // 覆盖模式，写入空内容
}

String SDManager::getTimestamp() {
    // 简单的时间戳实现，可以根据需要改进
    return String(millis());
}

// 固件升级相关
bool SDManager::hasFirmwareUpdate() {
    return fileExists("/firmware.bin");
}

bool SDManager::performFirmwareUpdate(const char* filename) {
    if (!fileExists(filename)) {
        debugPrint("固件文件不存在");
        return false;
    }
    
    File updateFile;
#ifdef SD_MODE_SPI
    updateFile = SD.open(filename, FILE_READ);
#else
    updateFile = SD_MMC.open(filename, FILE_READ);
#endif

    if (!updateFile) {
        debugPrint("无法打开固件文件");
        return false;
    }
    
    size_t updateSize = updateFile.size();
    
    if (updateSize == 0) {
        debugPrint("固件文件为空");
        updateFile.close();
        return false;
    }
    
    if (!Update.begin(updateSize)) {
        debugPrint("固件更新初始化失败");
        updateFile.close();
        return false;
    }
    
    size_t written = Update.writeStream(updateFile);
    
    if (written != updateSize) {
        debugPrint("固件写入不完整");
        updateFile.close();
        return false;
    }
    
    if (Update.end()) {
        debugPrint("固件更新成功，重启中...");
        updateFile.close();
        ESP.restart();
        return true;
    } else {
        debugPrint("固件更新失败");
        updateFile.close();
        return false;
    }
}
// 新卡初始化和数据记录功能
bool SDManager::initializeNewCard() {
    if (!_initialized) {
        debugPrint("SD卡未初始化，无法执行初始化操作");
        return false;
    }
    
    debugPrint("开始初始化新SD卡...");
    
    // 创建目录结构
    bool success = true;
    success &= createDir("/config");
    success &= createDir("/logs");
    success &= createDir("/firmware");
    success &= createDir("/data");
    success &= createDir("/data/gps");
    success &= createDir("/data/imu");
    
    if (!success) {
        debugPrint("❌ 创建目录结构失败");
        return false;
    }
    
    // 写入README文件
    String readmeContent = "MotoBox Data Storage\n";
    readmeContent += "===================\n\n";
    readmeContent += "This SD card is used by MotoBox for data storage.\n";
    readmeContent += "Please do not modify or delete any files unless you know what you are doing.\n\n";
    readmeContent += "Directory structure:\n";
    readmeContent += "- /config: Configuration files\n";
    readmeContent += "- /logs: System logs\n";
    readmeContent += "- /firmware: Firmware update files\n";
    readmeContent += "- /data: Sensor data\n";
    readmeContent += "  - /data/gps: GPS data in GeoJSON format\n";
    readmeContent += "  - /data/imu: IMU and compass data\n\n";
    readmeContent += "Created: " + getTimestamp() + "\n";
    
    if (!writeFile("/README.txt", readmeContent.c_str())) {
        debugPrint("❌ 写入README文件失败");
        return false;
    }
    
    // 写入设备信息
    if (!writeDeviceInfo()) {
        debugPrint("❌ 写入设备信息失败");
        return false;
    }
    
    // 创建空的WiFi配置文件模板
    StaticJsonDocument<256> wifiConfig;
    wifiConfig["ssid"] = "";
    wifiConfig["password"] = "";
    
    String wifiConfigStr;
    serializeJson(wifiConfig, wifiConfigStr);
    
    if (!writeFile("/config/wifi.json.template", wifiConfigStr.c_str())) {
        debugPrint("❌ 写入WiFi配置模板失败");
        // 非致命错误，继续执行
    }
    
    debugPrint("✅ 新SD卡初始化成功");
    return true;
}

bool SDManager::writeDeviceInfo() {
    if (!_initialized) {
        debugPrint("SD卡未初始化，无法写入设备信息");
        return false;
    }
    
    debugPrint("写入设备信息...");
    
    // 创建设备信息JSON
    StaticJsonDocument<512> deviceInfo;
    deviceInfo["device_id"] = device_state.device_id;
    deviceInfo["firmware_version"] = device_state.device_firmware_version;
    deviceInfo["hardware_version"] = device_state.device_hardware_version;
    deviceInfo["creation_time"] = getTimestamp();
    
    // 添加硬件配置信息
    JsonObject hardware = deviceInfo.createNestedObject("hardware");
    hardware["imu"] = device_state.imuReady;
    hardware["compass"] = device_state.compassReady;
    hardware["gps"] = device_state.gpsReady;
    hardware["gps_type"] = gpsManager.getType();
    hardware["wifi"] = device_state.wifiConnected;
    hardware["ble"] = device_state.bleConnected;
    
    // 添加SD卡信息
    JsonObject storage = deviceInfo.createNestedObject("storage");
    storage["size_mb"] = getCardSizeMB();
    storage["free_mb"] = getFreeSpaceMB();
    
    // 序列化为字符串
    String deviceInfoStr;
    serializeJsonPretty(deviceInfo, deviceInfoStr);
    
    // 写入文件
    if (!writeFile("/device_info.json", deviceInfoStr.c_str())) {
        debugPrint("❌ 写入设备信息失败");
        return false;
    }
    
    debugPrint("✅ 设备信息写入成功");
    return true;
}

bool SDManager::logGPSData(double latitude, double longitude, double altitude, 
                          float speed, float course, int satellites, float hdop, 
                          const String& timestamp) {
    if (!_initialized) {
        debugPrint("SD卡未初始化，无法记录GPS数据");
        return false;
    }
    
    // 检查GPS数据有效性
    if (latitude == 0 && longitude == 0) {
        // 无效的GPS数据，不记录
        return false;
    }
    
    // 创建文件名 - 使用日期作为文件名
    String date = timestamp.substring(0, 10); // 假设时间戳格式为 YYYY-MM-DD HH:MM:SS
    if (date.length() < 10) {
        // 使用当前毫秒数作为备用
        date = String(millis());
    }
    String filename = "/data/gps/" + date + ".geojson";
    
    // 检查文件是否存在，如果不存在则创建GeoJSON头
    bool fileExists = this->fileExists(filename.c_str());
    if (!fileExists) {
        // 创建新的GeoJSON文件
        String header = "{\n";
        header += "  \"type\": \"FeatureCollection\",\n";
        header += "  \"features\": [\n";
        
        if (!writeFile(filename.c_str(), header.c_str())) {
            debugPrint("❌ 创建GeoJSON文件失败: " + filename);
            return false;
        }
    } else {
        // 检查文件大小，如果太大则创建新文件
        File file;
#ifdef SD_MODE_SPI
        file = SD.open(filename.c_str(), FILE_READ);
#else
        file = SD_MMC.open(filename.c_str(), FILE_READ);
#endif
        if (file && file.size() > 5 * 1024 * 1024) { // 5MB限制
            file.close();
            // 创建新文件名
            String newFilename = "/data/gps/" + date + "_" + String(millis()) + ".geojson";
            String header = "{\n";
            header += "  \"type\": \"FeatureCollection\",\n";
            header += "  \"features\": [\n";
            
            if (!writeFile(newFilename.c_str(), header.c_str())) {
                debugPrint("❌ 创建新GeoJSON文件失败: " + newFilename);
                return false;
            }
            filename = newFilename;
            fileExists = false;
        } else if (file) {
            file.close();
        }
    }
    
    // 创建GeoJSON特征
    String feature = fileExists ? ",\n" : ""; // 如果文件已存在，添加逗号分隔
    feature += "    {\n";
    feature += "      \"type\": \"Feature\",\n";
    feature += "      \"geometry\": {\n";
    feature += "        \"type\": \"Point\",\n";
    feature += "        \"coordinates\": [" + String(longitude, 6) + ", " + String(latitude, 6) + ", " + String(altitude, 2) + "]\n";
    feature += "      },\n";
    feature += "      \"properties\": {\n";
    feature += "        \"timestamp\": \"" + timestamp + "\",\n";
    feature += "        \"speed\": " + String(speed, 2) + ",\n";
    feature += "        \"course\": " + String(course, 2) + ",\n";
    feature += "        \"satellites\": " + String(satellites) + ",\n";
    feature += "        \"hdop\": " + String(hdop, 2) + "\n";
    feature += "      }\n";
    feature += "    }";
    
    // 追加到文件
    if (!writeFile(filename.c_str(), feature.c_str(), true)) {
        debugPrint("❌ 写入GPS数据失败");
        return false;
    }
    
    return true;
}

bool SDManager::logIMUData(float ax, float ay, float az, float gx, float gy, float gz, 
                          float heading, const String& timestamp) {
    if (!_initialized) {
        debugPrint("SD卡未初始化，无法记录IMU数据");
        return false;
    }
    
    // 创建文件名 - 使用日期作为文件名
    String date = timestamp.substring(0, 10); // 假设时间戳格式为 YYYY-MM-DD HH:MM:SS
    if (date.length() < 10) {
        // 使用当前毫秒数作为备用
        date = String(millis());
    }
    String filename = "/data/imu/" + date + ".csv";
    
    // 检查文件是否存在，如果不存在则创建CSV头
    if (!fileExists(filename.c_str())) {
        // 创建新的CSV文件
        String header = "timestamp,ax,ay,az,gx,gy,gz,heading\n";
        
        if (!writeFile(filename.c_str(), header.c_str())) {
            debugPrint("❌ 创建IMU数据文件失败: " + filename);
            return false;
        }
    } else {
        // 检查文件大小，如果太大则创建新文件
        File file;
#ifdef SD_MODE_SPI
        file = SD.open(filename.c_str(), FILE_READ);
#else
        file = SD_MMC.open(filename.c_str(), FILE_READ);
#endif
        if (file && file.size() > 5 * 1024 * 1024) { // 5MB限制
            file.close();
            // 创建新文件名
            String newFilename = "/data/imu/" + date + "_" + String(millis()) + ".csv";
            String header = "timestamp,ax,ay,az,gx,gy,gz,heading\n";
            
            if (!writeFile(newFilename.c_str(), header.c_str())) {
                debugPrint("❌ 创建新IMU数据文件失败: " + newFilename);
                return false;
            }
            filename = newFilename;
        } else if (file) {
            file.close();
        }
    }
    
    // 创建CSV行
    String dataLine = timestamp + ",";
    dataLine += String(ax, 4) + ",";
    dataLine += String(ay, 4) + ",";
    dataLine += String(az, 4) + ",";
    dataLine += String(gx, 4) + ",";
    dataLine += String(gy, 4) + ",";
    dataLine += String(gz, 4) + ",";
    dataLine += String(heading, 2) + "\n";
    
    // 追加到文件
    if (!writeFile(filename.c_str(), dataLine.c_str(), true)) {
        debugPrint("❌ 写入IMU数据失败");
        return false;
    }
    
    return true;
}
// 状态监控和管理
void SDManager::updateStatus() {
    if (!_initialized) {
        // 尝试重新初始化SD卡
        static unsigned long lastRetryTime = 0;
        unsigned long currentTime = millis();
        
        // 每30秒尝试一次重新初始化
        if (currentTime - lastRetryTime > 30000) {
            lastRetryTime = currentTime;
            debugPrint("尝试重新初始化SD卡...");
            begin();
        }
        return;
    }
    
    // 更新设备状态中的SD卡信息
    device_state.sdCardReady = _initialized;
    device_state.sdCardSizeMB = getCardSizeMB();
    device_state.sdCardFreeMB = getFreeSpaceMB();
    
    // 检查SD卡健康状态
    static unsigned long lastHealthCheckTime = 0;
    unsigned long currentTime = millis();
    
    // 每5分钟检查一次SD卡健康状态
    if (currentTime - lastHealthCheckTime > 300000) {
        lastHealthCheckTime = currentTime;
        
        // 简单的健康检查 - 尝试写入和读取测试文件
        const char* testFile = "/.sd_health_check";
        const char* testContent = "SD Health Check";
        
        bool healthOK = writeFile(testFile, testContent) && 
                        readFile(testFile) == testContent &&
                        deleteFile(testFile);
        
        if (!healthOK) {
            debugPrint("⚠️ SD卡健康检查失败，尝试修复...");
            // 尝试修复 - 重新初始化
            end();
            delay(100);
            begin();
        }
    }
}

// 低功耗管理
void SDManager::prepareForSleep() {
    if (_initialized) {
        debugPrint("为深度睡眠准备SD卡");
        end();
    }
}

void SDManager::restoreFromSleep() {
    debugPrint("从深度睡眠恢复SD卡");
    begin();
}

void SDManager::enterLowPowerMode() {
    if (_initialized && !_lowPowerMode) {
        debugPrint("进入低功耗模式");
        end();
        _lowPowerMode = true;
    }
}

void SDManager::exitLowPowerMode() {
    if (_lowPowerMode) {
        debugPrint("退出低功耗模式");
        _lowPowerMode = false;
        begin();
    }
}

// SPI性能优化
void SDManager::optimizeSPIPerformance() {
#ifdef SD_MODE_SPI
    if (!_initialized) {
        debugPrint("SD卡未初始化，无法优化性能");
        return;
    }
    
    debugPrint("开始SPI性能优化...");
    
    const uint32_t testFrequencies[] = {20000000, 16000000, 10000000, 8000000, 4000000};
    const char* freqNames[] = {"20MHz", "16MHz", "10MHz", "8MHz", "4MHz"};
    uint32_t bestFrequency = _optimalSPIFrequency;
    unsigned long bestTime = ULONG_MAX;
    
    for (int i = 0; i < 5; i++) {
        debugPrint(String("测试频率: ") + freqNames[i]);
        
        // 重新初始化SD卡使用新频率
        SD.end();
        delay(100);
        SPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
        
        if (SD.begin(SD_CS_PIN, SPI, testFrequencies[i])) {
            // 性能测试 - 写入和读取测试文件
            const char* testFile = "/.perf_test";
            const char* testContent = "Performance Test Data 1234567890";
            
            unsigned long startTime = millis();
            
            bool testOK = true;
            for (int j = 0; j < 10; j++) {
                if (!writeFile(testFile, testContent) || 
                    readFile(testFile) != testContent) {
                    testOK = false;
                    break;
                }
            }
            
            unsigned long endTime = millis();
            deleteFile(testFile);
            
            if (testOK) {
                unsigned long testTime = endTime - startTime;
                debugPrint(String("频率 ") + freqNames[i] + " 测试时间: " + testTime + "ms");
                
                if (testTime < bestTime) {
                    bestTime = testTime;
                    bestFrequency = testFrequencies[i];
                }
            } else {
                debugPrint(String("频率 ") + freqNames[i] + " 测试失败");
            }
        } else {
            debugPrint(String("频率 ") + freqNames[i] + " 初始化失败");
        }
    }
    
    // 使用最佳频率重新初始化
    _optimalSPIFrequency = bestFrequency;
    SD.end();
    delay(100);
    SPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
    SD.begin(SD_CS_PIN, SPI, _optimalSPIFrequency);
    
    debugPrint(String("SPI性能优化完成，最佳频率: ") + _optimalSPIFrequency + " Hz");
#endif
}

// 硬件诊断方法实现
bool SDManager::validateSPIPins() {
#ifdef SD_MODE_SPI
    // 检查引脚是否在有效范围内
    if (SD_CS_PIN < 0 || SD_CS_PIN > 48 ||
        SD_MOSI_PIN < 0 || SD_MOSI_PIN > 48 ||
        SD_MISO_PIN < 0 || SD_MISO_PIN > 48 ||
        SD_SCK_PIN < 0 || SD_SCK_PIN > 48) {
        debugPrint("❌ SPI引脚超出有效范围 (0-48)");
        return false;
    }
    
    // 检查引脚是否重复
    int pins[] = {SD_CS_PIN, SD_MOSI_PIN, SD_MISO_PIN, SD_SCK_PIN};
    for (int i = 0; i < 4; i++) {
        for (int j = i + 1; j < 4; j++) {
            if (pins[i] == pins[j]) {
                debugPrint("❌ SPI引脚配置重复: " + String(pins[i]));
                return false;
            }
        }
    }
    
    // 检查是否与已知的保留引脚冲突
    int reservedPins[] = {0, 1, 3}; // Boot, TX, RX等
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 3; j++) {
            if (pins[i] == reservedPins[j]) {
                debugPrint("⚠️  警告: 引脚 " + String(pins[i]) + " 可能与系统功能冲突");
            }
        }
    }
    
    debugPrint("✅ SPI引脚配置验证通过");
    return true;
#else
    return true; // SDIO模式不需要验证引脚
#endif
}

bool SDManager::performDetailedHardwareCheck() {
#ifdef SD_MODE_SPI
    debugPrint("执行详细硬件检查...");
    
    // 检查CS引脚
    pinMode(SD_CS_PIN, OUTPUT);
    digitalWrite(SD_CS_PIN, HIGH);
    delay(10);
    if (digitalRead(SD_CS_PIN) != HIGH) {
        debugPrint("❌ CS引脚无法设置为HIGH");
        return false;
    }
    
    digitalWrite(SD_CS_PIN, LOW);
    delay(10);
    if (digitalRead(SD_CS_PIN) != LOW) {
        debugPrint("❌ CS引脚无法设置为LOW");
        return false;
    }
    
    // 恢复CS引脚状态
    digitalWrite(SD_CS_PIN, HIGH);
    
    // 检查MISO引脚（应该有上拉电阻）
    pinMode(SD_MISO_PIN, INPUT_PULLUP);
    delay(10);
    if (digitalRead(SD_MISO_PIN) != HIGH) {
        debugPrint("⚠️ MISO引脚无法上拉，可能需要外部上拉电阻");
        // 这不是致命错误，继续执行
    }
    
    // 初始化SPI总线
    SPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
    
    // 发送一些时钟脉冲以稳定SD卡
    digitalWrite(SD_CS_PIN, HIGH);
    for (int i = 0; i < 10; i++) {
        SPI.transfer(0xFF);
    }
    
    debugPrint("✅ 硬件检查通过");
    return true;
#else
    return true; // SDIO模式不需要详细硬件检查
#endif
}

bool SDManager::verifySDCardOperation() {
    debugPrint("验证SD卡基本操作...");
    
    // 测试文件写入
    const char* testFile = "/sd_test.txt";
    const char* testContent = "SD Card Test";
    
    if (!writeFile(testFile, testContent)) {
        debugPrint("❌ SD卡写入测试失败");
        return false;
    }
    
    // 测试文件读取
    String readContent = readFile(testFile);
    if (readContent != testContent) {
        debugPrint("❌ SD卡读取测试失败");
        deleteFile(testFile);
        return false;
    }
    
    // 清理测试文件
    deleteFile(testFile);
    
    debugPrint("✅ SD卡基本操作验证通过");
    return true;
}

void SDManager::performHardwareDiagnostic() {
    Serial.println("\n=== SD卡硬件诊断 ===");
    
#ifdef SD_MODE_SPI
    Serial.println("[诊断] 使用SPI模式");
    Serial.printf("[诊断] 引脚配置: CS=%d, MOSI=%d, MISO=%d, SCK=%d\n", 
                  SD_CS_PIN, SD_MOSI_PIN, SD_MISO_PIN, SD_SCK_PIN);
    
    // 检查引脚状态
    Serial.println("[诊断] 检查引脚状态:");
    pinMode(SD_CS_PIN, INPUT_PULLUP);
    pinMode(SD_MISO_PIN, INPUT_PULLUP);
    delay(10);
    
    Serial.printf("[诊断] CS引脚状态: %s\n", digitalRead(SD_CS_PIN) ? "HIGH" : "LOW");
    Serial.printf("[诊断] MISO引脚状态: %s\n", digitalRead(SD_MISO_PIN) ? "HIGH" : "LOW");
    
    // 检查SPI总线
    Serial.println("[诊断] 检查SPI总线...");
    SPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
    
    // 尝试发送简单的SPI命令
    digitalWrite(SD_CS_PIN, LOW);
    SPI.transfer(0xFF);
    digitalWrite(SD_CS_PIN, HIGH);
    
    Serial.println("[诊断] SPI总线测试完成");
    
#else
    Serial.println("[诊断] 使用SDIO模式");
    Serial.println("[诊断] SDIO引脚为硬件固定，无需检查");
#endif
    
    Serial.println("=== 诊断完成 ===\n");
}

// 串口命令处理
bool SDManager::handleSerialCommand(const String& command) {
    if (!_initialized) {
        debugPrint("SD卡未初始化，无法处理命令");
        return false;
    }
    
    if (command.startsWith("sd.list")) {
        // 列出根目录内容
        listDir("/");
        return true;
    } else if (command.startsWith("sd.info")) {
        // 显示SD卡信息
        printCardInfo();
        return true;
    } else if (command.startsWith("sd.test")) {
        // 执行SD卡测试
        bool result = verifySDCardOperation();
        Serial.printf("SD卡测试结果: %s\n", result ? "通过" : "失败");
        return true;
    } else if (command.startsWith("sd.format")) {
        // 格式化SD卡（危险操作，暂不实现）
        Serial.println("格式化功能暂未实现");
        return true;
    } else if (command.startsWith("sd.optimize")) {
        // 优化SPI性能
        optimizeSPIPerformance();
        return true;
    } else if (command == "yes_format") {
        // 确认格式化SD卡
        Serial.println("确认格式化SD卡 - 此功能暂未实现");
        return true;
    }
    
    return false; // 未识别的命令
}

void SDManager::showHelp() {
    Serial.println("SD卡管理命令:");
    Serial.println("  sd.list    - 列出根目录内容");
    Serial.println("  sd.info    - 显示SD卡信息");
    Serial.println("  sd.test    - 执行SD卡测试");
    Serial.println("  sd.optimize - 优化SPI性能");
}

void SDManager::debugPrint(const String& message) {
    if (_debug) {
        Serial.println("[SD] " + message);
    }
}

#endif // ENABLE_SDCARD
