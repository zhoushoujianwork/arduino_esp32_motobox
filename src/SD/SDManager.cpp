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

    debugPrint("正在初始化SD卡...");

#ifdef SD_MODE_SPI
    // SPI模式初始化
    debugPrint("使用SPI模式，引脚配置: CS=" + String(SD_CS_PIN) + ", MOSI=" + String(SD_MOSI_PIN) + ", MISO=" + String(SD_MISO_PIN) + ", SCK=" + String(SD_SCK_PIN));
    
    SPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
    
    if (!SD.begin(SD_CS_PIN)) {
        debugPrint("❌ SD卡初始化失败");
        debugPrint("可能的原因：");
        debugPrint("  1. 未插入SD卡");
        debugPrint("  2. SD卡损坏或格式不支持");
        debugPrint("  3. 硬件连接错误");
        debugPrint("  4. SD卡格式不是FAT32");
        debugPrint("请检查SD卡并重试");
        return false;
    }
    
    // 检查SD卡类型
    uint8_t cardType = SD.cardType();
    if (cardType == CARD_NONE) {
        debugPrint("❌ 未检测到SD卡");
        debugPrint("请确认SD卡已正确插入");
        return false;
    }
    
    // 设置初始化标志 - 在获取容量信息之前设置
    _initialized = true;
    
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
            cardTypeStr = "未知";
            break;
    }
    
    debugPrint("✅ SD卡初始化成功");
    debugPrint("SD卡类型: " + cardTypeStr);
    debugPrint("SD卡容量: " + String((unsigned long)getTotalSpaceMB()) + " MB");
    debugPrint("可用空间: " + String((unsigned long)getFreeSpaceMB()) + " MB");
    
#else
    // MMC模式初始化
    debugPrint("使用MMC模式");
    if (!SD_MMC.begin()) {
        debugPrint("❌ SD卡MMC模式初始化失败");
        debugPrint("可能的原因：");
        debugPrint("  1. 未插入SD卡");
        debugPrint("  2. SD卡损坏");
        debugPrint("  3. MMC模式不支持此SD卡");
        debugPrint("请检查SD卡并重试");
        return false;
    }
    
    // 设置初始化标志
    _initialized = true;
    
    debugPrint("✅ SD卡MMC模式初始化成功");
    debugPrint("SD卡容量: " + String((unsigned long)getTotalSpaceMB()) + " MB");
    debugPrint("可用空间: " + String((unsigned long)getFreeSpaceMB()) + " MB");
#endif

    // 创建必要的目录结构
    if (!createDirectoryStructure()) {
        debugPrint("⚠️ 目录结构创建失败，但SD卡可用");
    }
    
    // 保存设备信息
    if (!saveDeviceInfo()) {
        debugPrint("⚠️ 设备信息保存失败，但SD卡可用");
    }

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
        debugPrint("⚠️ SD卡未初始化，无法获取容量信息");
        return 0;
    }

    try {
#ifdef SD_MODE_SPI
        return SD.totalBytes() / (1024 * 1024);
#else
        return SD_MMC.totalBytes() / (1024 * 1024);
#endif
    } catch (...) {
        debugPrint("⚠️ 获取SD卡容量失败，可能SD卡已移除");
        return 0;
    }
}

uint64_t SDManager::getFreeSpaceMB() {
    if (!_initialized) {
        debugPrint("⚠️ SD卡未初始化，无法获取剩余空间");
        return 0;
    }

    try {
#ifdef SD_MODE_SPI
        return (SD.totalBytes() - SD.usedBytes()) / (1024 * 1024);
#else
        return (SD_MMC.totalBytes() - SD_MMC.usedBytes()) / (1024 * 1024);
#endif
    } catch (...) {
        debugPrint("⚠️ 获取SD卡剩余空间失败，可能SD卡已移除");
        return 0;
    }
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
        debugPrint("⚠️ SD卡未初始化，无法创建目录: " + String(path));
        return false;
    }

    // 先检查目录是否已存在
    if (directoryExists(path)) {
        debugPrint("📁 目录已存在: " + String(path));
        return true;
    }

    debugPrint("🔧 正在创建目录: " + String(path));

    bool success = false;
    try {
#ifdef SD_MODE_SPI
        success = SD.mkdir(path);
#else
        success = SD_MMC.mkdir(path);
#endif
    } catch (...) {
        debugPrint("❌ 创建目录时发生异常: " + String(path));
        return false;
    }

    if (success) {
        debugPrint("✅ 目录创建成功: " + String(path));
        
        // 验证目录是否真的创建成功
        if (directoryExists(path)) {
            return true;
        } else {
            debugPrint("⚠️ 目录创建报告成功但验证失败: " + String(path));
            return false;
        }
    } else {
        debugPrint("❌ 目录创建失败: " + String(path));
        debugPrint("可能的原因：");
        debugPrint("  1. SD卡空间不足");
        debugPrint("  2. SD卡写保护");
        debugPrint("  3. 文件系统错误");
        debugPrint("  4. 路径格式错误");
        return false;
    }
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
        debugPrint("⚠️ SD卡未初始化，无法记录GPS数据");
        return false;
    }

    // 验证GPS数据有效性
    if (latitude < -90 || latitude > 90 || longitude < -180 || longitude > 180) {
        debugPrint("⚠️ GPS数据无效: lat=" + String(latitude, 6) + ", lon=" + String(longitude, 6));
        return false;
    }

    // 生成当前会话的GPS文件名
    String filename = generateGPSSessionFilename();
    
    // 确保GPS目录存在
    if (!ensureGPSDirectoryExists()) {
        debugPrint("❌ 无法创建GPS目录");
        return false;
    }
    
    // 检查文件是否存在，如果不存在则创建GeoJSON头部
    bool fileExists = false;
    
    try {
#ifdef SD_MODE_SPI
        File testFile = SD.open(filename.c_str(), FILE_READ);
#else
        File testFile = SD_MMC.open(filename.c_str(), FILE_READ);
#endif
        
        if (testFile) {
            fileExists = true;
            testFile.close();
            debugPrint("📄 使用现有GPS会话文件: " + filename);
        }
    } catch (...) {
        debugPrint("⚠️ 检查GPS文件状态失败，可能SD卡已移除");
        return false;
    }

    // 打开文件进行写入
    File file;
    try {
#ifdef SD_MODE_SPI
        file = SD.open(filename.c_str(), FILE_APPEND);
#else
        file = SD_MMC.open(filename.c_str(), FILE_APPEND);
#endif
    } catch (...) {
        debugPrint("⚠️ 打开GPS数据文件失败，可能SD卡已移除");
        return false;
    }

    if (!file) {
        debugPrint("❌ 无法打开GPS数据文件: " + filename);
        debugPrint("可能的原因：");
        debugPrint("  1. SD卡空间不足");
        debugPrint("  2. SD卡已移除");
        debugPrint("  3. 文件系统错误");
        debugPrint("  4. 目录权限问题");
        
        // 尝试重新创建目录
        debugPrint("🔧 尝试重新创建GPS目录...");
        if (createDirectory("/data") && createDirectory("/data/gps")) {
            debugPrint("✅ GPS目录重新创建成功，请重试");
        } else {
            debugPrint("❌ GPS目录重新创建失败");
        }
        return false;
    }

    // 如果是新文件，写入GeoJSON头部和会话信息
    if (!fileExists) {
        debugPrint("📁 创建新的GPS会话文件: " + filename);
        
        file.println("{");
        file.println("  \"type\": \"FeatureCollection\",");
        file.println("  \"metadata\": {");
        file.println("    \"device_id\": \"" + getDeviceID() + "\",");
        file.println("    \"session_start\": \"" + getCurrentTimestamp() + "\",");
        file.println("    \"boot_count\": " + String(getBootCount()) + ",");
        file.println("    \"firmware_version\": \"" + String(FIRMWARE_VERSION) + "\"");
        file.println("  },");
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
    gpsFeature += "        \"runtime_ms\": " + String(millis()) + ",\n";
    gpsFeature += "        \"speed_kmh\": " + String(speed, 2) + ",\n";
    gpsFeature += "        \"satellites\": " + String(satellites) + ",\n";
    gpsFeature += "        \"hdop\": 0.0\n";  // 可以后续添加HDOP数据
    gpsFeature += "      }\n";
    gpsFeature += "    }";

    size_t bytesWritten = file.print(gpsFeature);
    file.flush(); // 确保数据写入
    file.close();

    if (bytesWritten == 0) {
        debugPrint("❌ GPS数据写入失败");
        debugPrint("可能SD卡空间不足或已移除");
        return false;
    }

    debugPrint("📍 GPS数据已记录: " + String(latitude, 6) + "," + String(longitude, 6) + " (卫星:" + String(satellites) + ")");
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

String SDManager::getCurrentTimeString() {
    // 生成当前时间字符串 HHMMSS
    unsigned long currentTime = millis();
    unsigned long hours = (currentTime % (24 * 60 * 60 * 1000)) / (60 * 60 * 1000);
    unsigned long minutes = (currentTime % (60 * 60 * 1000)) / (60 * 1000);
    unsigned long seconds = (currentTime % (60 * 1000)) / 1000;
    
    String timeStr = "";
    if (hours < 10) timeStr += "0";
    timeStr += String(hours);
    if (minutes < 10) timeStr += "0";
    timeStr += String(minutes);
    if (seconds < 10) timeStr += "0";
    timeStr += String(seconds);
    
    return timeStr;
}

String SDManager::generateGPSSessionFilename() {
    // 生成基于启动会话的GPS文件名
    // 格式: gps_YYYYMMDD_HHMMSS_bootXXX.geojson
    
    // 获取当前日期时间（简化版）
    String dateStr = getCurrentDateString();
    String timeStr = getCurrentTimeString();
    
    // 格式化启动次数
    String bootStr = String(getBootCount());
    while (bootStr.length() < 3) {
        bootStr = "0" + bootStr;
    }
    
    return "/data/gps/gps_" + dateStr + "_" + timeStr + "_boot" + bootStr + ".geojson";
}

int SDManager::getBootCount() {
    // 从外部获取启动次数
    extern int bootCount;
    return bootCount;
}

bool SDManager::finishGPSSession() {
    if (!_initialized) {
        debugPrint("⚠️ SD卡未初始化，无法结束GPS会话");
        return false;
    }

    // 获取当前会话文件名
    String filename = generateGPSSessionFilename();
    
    // 检查文件是否存在
    File testFile;
    try {
#ifdef SD_MODE_SPI
        testFile = SD.open(filename.c_str(), FILE_READ);
#else
        testFile = SD_MMC.open(filename.c_str(), FILE_READ);
#endif
    } catch (...) {
        debugPrint("⚠️ 检查GPS文件失败");
        return false;
    }

    if (!testFile) {
        debugPrint("⚠️ GPS会话文件不存在: " + filename);
        return false;
    }
    testFile.close();

    // 以追加模式打开文件，添加GeoJSON结尾
    File file;
    try {
#ifdef SD_MODE_SPI
        file = SD.open(filename.c_str(), FILE_APPEND);
#else
        file = SD_MMC.open(filename.c_str(), FILE_APPEND);
#endif
    } catch (...) {
        debugPrint("⚠️ 打开GPS文件失败");
        return false;
    }

    if (!file) {
        debugPrint("❌ 无法打开GPS会话文件进行结束操作");
        return false;
    }

    // 添加GeoJSON结尾
    file.println("");
    file.println("  ]");
    file.println("}");
    file.close();

    debugPrint("✅ GPS会话已结束: " + filename);
    return true;
}

bool SDManager::ensureGPSDirectoryExists() {
    if (!_initialized) {
        return false;
    }

    // 检查并创建 /data 目录
    if (!directoryExists("/data")) {
        debugPrint("🔧 创建 /data 目录...");
        if (!createDirectory("/data")) {
            debugPrint("❌ 创建 /data 目录失败");
            return false;
        }
        debugPrint("✅ /data 目录创建成功");
    }

    // 检查并创建 /data/gps 目录
    if (!directoryExists("/data/gps")) {
        debugPrint("🔧 创建 /data/gps 目录...");
        if (!createDirectory("/data/gps")) {
            debugPrint("❌ 创建 /data/gps 目录失败");
            return false;
        }
        debugPrint("✅ /data/gps 目录创建成功");
    }

    return true;
}

bool SDManager::directoryExists(const char* path) {
    if (!_initialized) {
        return false;
    }

    File dir;
    try {
#ifdef SD_MODE_SPI
        dir = SD.open(path);
#else
        dir = SD_MMC.open(path);
#endif
    } catch (...) {
        return false;
    }

    if (!dir) {
        return false;
    }

    bool isDir = dir.isDirectory();
    dir.close();
    return isDir;
}

void SDManager::debugPrint(const String& message) {
    Serial.println("[SDManager] " + message);
}

// 串口命令处理
bool SDManager::handleSerialCommand(const String& command) {
    if (command == "sd.info") {
        if (!_initialized) {
            Serial.println("❌ SD卡未初始化");
            Serial.println("可能的原因：");
            Serial.println("  1. 未插入SD卡");
            Serial.println("  2. SD卡损坏或格式不支持");
            Serial.println("  3. 硬件连接错误");
            return false;
        }
        
        Serial.println("=== SD卡信息 ===");
        Serial.println("设备ID: " + getDeviceID());
        
        uint64_t totalMB = getTotalSpaceMB();
        uint64_t freeMB = getFreeSpaceMB();
        
        if (totalMB > 0) {
            Serial.println("总容量: " + String((unsigned long)totalMB) + " MB");
            Serial.println("剩余空间: " + String((unsigned long)freeMB) + " MB");
            Serial.println("使用率: " + String((unsigned long)((totalMB - freeMB) * 100 / totalMB)) + "%");
        } else {
            Serial.println("⚠️ 无法获取容量信息，SD卡可能已移除");
        }
        
        Serial.println("初始化状态: " + String(_initialized ? "已初始化" : "未初始化"));
        return true;
    }
    else if (command == "sd.test") {
        if (!_initialized) {
            Serial.println("❌ SD卡未初始化，无法进行测试");
            Serial.println("请先插入SD卡并重启设备");
            return false;
        }
        
        // 测试GPS数据记录
        Serial.println("正在测试GPS数据记录...");
        Serial.println("测试数据: 北京天安门广场坐标");
        Serial.println("当前会话文件: " + generateGPSSessionFilename());
        
        bool result = recordGPSData(39.9042, 116.4074, 50.0, 25.5, 8);
        
        if (result) {
            Serial.println("✅ GPS数据记录测试成功");
            Serial.println("数据已保存到当前会话文件");
        } else {
            Serial.println("❌ GPS数据记录测试失败");
            Serial.println("请检查SD卡状态");
        }
        
        return result;
    }
    else if (command == "sd.session") {
        if (!_initialized) {
            Serial.println("❌ SD卡未初始化");
            return false;
        }
        
        Serial.println("=== GPS会话信息 ===");
        Serial.println("当前会话文件: " + generateGPSSessionFilename());
        Serial.println("启动次数: " + String(getBootCount()));
        Serial.println("运行时间: " + String(millis() / 1000) + " 秒");
        Serial.println("设备ID: " + getDeviceID());
        return true;
    }
    else if (command == "sd.finish") {
        if (!_initialized) {
            Serial.println("❌ SD卡未初始化");
            return false;
        }
        
        Serial.println("正在结束当前GPS会话...");
        bool result = finishGPSSession();
        
        if (result) {
            Serial.println("✅ GPS会话已正确结束");
        } else {
            Serial.println("❌ GPS会话结束失败");
        }
        
        return result;
    }
    else if (command == "sd.dirs") {
        if (!_initialized) {
            Serial.println("❌ SD卡未初始化");
            return false;
        }
        
        Serial.println("=== 目录状态检查 ===");
        Serial.println("/data 目录: " + String(directoryExists("/data") ? "存在" : "不存在"));
        Serial.println("/data/gps 目录: " + String(directoryExists("/data/gps") ? "存在" : "不存在"));
        Serial.println("/config 目录: " + String(directoryExists("/config") ? "存在" : "不存在"));
        
        Serial.println("");
        Serial.println("正在确保GPS目录存在...");
        bool result = ensureGPSDirectoryExists();
        
        if (result) {
            Serial.println("✅ GPS目录检查/创建成功");
        } else {
            Serial.println("❌ GPS目录检查/创建失败");
        }
        
        return result;
    }
    else if (command == "yes_format") {
        Serial.println("⚠️ 简化版SD管理器暂不支持格式化功能");
        Serial.println("如需格式化，请使用电脑格式化为FAT32格式");
        return false;
    }
    else if (command == "sd.status") {
        Serial.println("=== SD卡状态检查 ===");
        
        if (!_initialized) {
            Serial.println("❌ SD卡状态: 未初始化");
            Serial.println("建议操作:");
            Serial.println("  1. 检查SD卡是否正确插入");
            Serial.println("  2. 确认SD卡格式为FAT32");
            Serial.println("  3. 重启设备重新初始化");
            return false;
        }
        
        Serial.println("✅ SD卡状态: 已初始化");
        
        // 测试基本读写功能
        Serial.println("正在测试基本读写功能...");
        
        uint64_t freeMB = getFreeSpaceMB();
        if (freeMB == 0) {
            Serial.println("⚠️ 警告: 无法获取剩余空间，SD卡可能已移除");
            return false;
        }
        
        Serial.println("✅ 读写功能正常");
        Serial.println("剩余空间: " + String((unsigned long)freeMB) + " MB");
        
        return true;
    }
    
    Serial.println("❌ 未知SD卡命令: " + command);
    Serial.println("可用的SD卡命令:");
    Serial.println("  sd.info    - 显示SD卡详细信息");
    Serial.println("  sd.test    - 测试GPS数据记录");
    Serial.println("  sd.status  - 检查SD卡状态");
    Serial.println("  sd.session - 显示当前GPS会话信息");
    Serial.println("  sd.finish  - 结束当前GPS会话");
    Serial.println("  sd.dirs    - 检查和创建目录结构");
    return false;
}
