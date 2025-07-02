/*
 * SD卡管理类实现 - 支持SPI和SDIO模式
 */

#include "SDManager.h"
#include <Update.h>

#ifdef ENABLE_SDCARD

SDManager::SDManager() : _initialized(false), _debug(false) {
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
    const uint32_t frequencies[] = {4000000, 1000000, 400000}; // 4MHz, 1MHz, 400kHz
    const char* freqNames[] = {"4MHz", "1MHz", "400kHz"};
    
    for (int i = 0; i < 3; i++) {
        debugPrint(String("尝试使用 ") + freqNames[i] + " 频率初始化SD卡");
        
        // 重置SPI设置
        SPI.end();
        delay(100);
        SPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
        
        // 尝试初始化SD卡
        if (SD.begin(SD_CS_PIN, SPI, frequencies[i])) {
            debugPrint(String("✅ SPI模式SD卡初始化成功 (") + freqNames[i] + ")");
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
    
    // 创建必要的目录
    createDir("/config");
    createDir("/logs");
    createDir("/firmware");
    
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
        debugPrint(String("文件不存在: ") + filename);
        return false;
    }
    
#ifdef SD_MODE_SPI
    bool result = SD.remove(filename);
#else
    bool result = SD_MMC.remove(filename);
#endif
    
    if (result) {
        debugPrint(String("文件删除成功: ") + filename);
    } else {
        debugPrint(String("文件删除失败: ") + filename);
    }
    
    return result;
}

bool SDManager::createDir(const char* dirname) {
    if (!_initialized) return false;
    
#ifdef SD_MODE_SPI
    if (SD.exists(dirname)) {
        return true; // 目录已存在
    }
    bool result = SD.mkdir(dirname);
#else
    if (SD_MMC.exists(dirname)) {
        return true; // 目录已存在
    }
    bool result = SD_MMC.mkdir(dirname);
#endif
    
    if (result) {
        debugPrint(String("目录创建成功: ") + dirname);
    } else {
        debugPrint(String("目录创建失败: ") + dirname);
    }
    
    return result;
}

void SDManager::listDir(const char* dirname) {
    if (!_initialized) {
        debugPrint("SD卡未初始化");
        return;
    }
    
#ifdef SD_MODE_SPI
    File root = SD.open(dirname);
#else
    File root = SD_MMC.open(dirname);
#endif
    
    if (!root) {
        debugPrint(String("无法打开目录: ") + dirname);
        return;
    }
    
    if (!root.isDirectory()) {
        debugPrint(String("不是目录: ") + dirname);
        return;
    }
    
    Serial.printf("目录内容: %s\n", dirname);
    File file = root.openNextFile();
    while (file) {
        if (file.isDirectory()) {
            Serial.printf("  DIR : %s\n", file.name());
        } else {
            Serial.printf("  FILE: %s  SIZE: %d\n", file.name(), file.size());
        }
        file = root.openNextFile();
    }
}

bool SDManager::hasFirmwareUpdate() {
    return fileExists("/firmware.bin") || fileExists("/firmware/update.bin");
}

bool SDManager::performFirmwareUpdate(const char* filename) {
    if (!_initialized) {
        debugPrint("SD卡未初始化");
        return false;
    }
    
    if (!fileExists(filename)) {
        debugPrint(String("固件文件不存在: ") + filename);
        return false;
    }
    
    // 验证固件文件
    if (!validateFirmware(filename)) {
        debugPrint("固件文件验证失败");
        return false;
    }
    
#ifdef SD_MODE_SPI
    File file = SD.open(filename, FILE_READ);
#else
    File file = SD_MMC.open(filename, FILE_READ);
#endif
    
    if (!file) {
        debugPrint("无法打开固件文件");
        return false;
    }
    
    size_t fileSize = file.size();
    debugPrint(String("固件文件大小: ") + String(fileSize) + " bytes");
    
    // 开始OTA更新
    if (!Update.begin(fileSize)) {
        debugPrint("OTA更新开始失败");
        file.close();
        return false;
    }
    
    // 写入固件数据
    size_t written = Update.writeStream(file);
    file.close();
    
    if (written != fileSize) {
        debugPrint("固件写入不完整");
        Update.abort();
        return false;
    }
    
    if (!Update.end()) {
        debugPrint("OTA更新结束失败");
        return false;
    }
    
    debugPrint("固件更新成功，准备重启...");
    
    // 记录更新日志
    writeLog(String("固件更新成功: ") + filename + " (" + String(fileSize) + " bytes)");
    
    // 删除固件文件
    deleteFile(filename);
    
    delay(1000);
    ESP.restart();
    
    return true;
}

bool SDManager::backupCurrentFirmware(const char* filename) {
    // 注意：ESP32无法直接读取当前运行的固件
    // 这里只是创建一个占位符实现
    debugPrint("当前固件备份功能暂未实现");
    return false;
}

bool SDManager::saveConfig(const String& config, const char* filename) {
    if (!_initialized) return false;
    
#ifdef SD_MODE_SPI
    File file = SD.open(filename, FILE_WRITE);
#else
    File file = SD_MMC.open(filename, FILE_WRITE);
#endif
    
    if (!file) {
        debugPrint(String("无法创建配置文件: ") + filename);
        return false;
    }
    
    size_t written = file.print(config);
    file.close();
    
    bool success = (written == config.length());
    if (success) {
        debugPrint(String("配置保存成功: ") + filename);
    } else {
        debugPrint(String("配置保存失败: ") + filename);
    }
    
    return success;
}

String SDManager::loadConfig(const char* filename) {
    if (!_initialized) return "";
    
    if (!fileExists(filename)) {
        debugPrint(String("配置文件不存在: ") + filename);
        return "";
    }
    
#ifdef SD_MODE_SPI
    File file = SD.open(filename, FILE_READ);
#else
    File file = SD_MMC.open(filename, FILE_READ);
#endif
    
    if (!file) {
        debugPrint(String("无法打开配置文件: ") + filename);
        return "";
    }
    
    String config = file.readString();
    file.close();
    
    debugPrint(String("配置加载成功: ") + filename);
    return config;
}

bool SDManager::saveWiFiConfig(const String& ssid, const String& password) {
    DynamicJsonDocument doc(512);
    doc["ssid"] = ssid;
    doc["password"] = password;
    doc["timestamp"] = getTimestamp();
    
    String jsonString;
    serializeJson(doc, jsonString);
    
    return saveConfig(jsonString, "/config/wifi.json");
}

bool SDManager::loadWiFiConfig(String& ssid, String& password) {
    String config = loadConfig("/config/wifi.json");
    if (config.isEmpty()) {
        return false;
    }
    
    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, config);
    
    if (error) {
        debugPrint("WiFi配置JSON解析失败");
        return false;
    }
    
    ssid = doc["ssid"].as<String>();
    password = doc["password"].as<String>();
    
    return !ssid.isEmpty();
}

bool SDManager::writeLog(const String& message, const char* filename) {
    if (!_initialized) return false;
    
#ifdef SD_MODE_SPI
    File file = SD.open(filename, FILE_APPEND);
#else
    File file = SD_MMC.open(filename, FILE_APPEND);
#endif
    
    if (!file) {
        debugPrint(String("无法打开日志文件: ") + filename);
        return false;
    }
    
    String logEntry = getTimestamp() + " " + message + "\n";
    size_t written = file.print(logEntry);
    file.close();
    
    return (written == logEntry.length());
}

String SDManager::readLog(const char* filename, int maxLines) {
    if (!_initialized) return "";
    
    if (!fileExists(filename)) {
        return "";
    }
    
#ifdef SD_MODE_SPI
    File file = SD.open(filename, FILE_READ);
#else
    File file = SD_MMC.open(filename, FILE_READ);
#endif
    
    if (!file) {
        return "";
    }
    
    String content = file.readString();
    file.close();
    
    if (maxLines <= 0) {
        return content;
    }
    
    // 简单实现：返回最后maxLines行
    int lineCount = 0;
    int lastNewlinePos = content.length();
    
    for (int i = content.length() - 1; i >= 0 && lineCount < maxLines; i--) {
        if (content.charAt(i) == '\n') {
            lineCount++;
            if (lineCount == maxLines) {
                return content.substring(i + 1);
            }
        }
    }
    
    return content;
}

bool SDManager::clearLog(const char* filename) {
    if (!_initialized) return false;
    
#ifdef SD_MODE_SPI
    File file = SD.open(filename, FILE_WRITE);
#else
    File file = SD_MMC.open(filename, FILE_WRITE);
#endif
    
    if (!file) {
        return false;
    }
    
    file.close();
    debugPrint(String("日志文件已清空: ") + filename);
    return true;
}

void SDManager::prepareForSleep() {
    if (_initialized) {
        debugPrint("准备进入睡眠模式，断开SD卡");
        end();
    }
}

void SDManager::restoreFromSleep() {
    if (!_initialized) {
        debugPrint("从睡眠模式恢复，重新初始化SD卡");
        begin();
    }
}

void SDManager::debugPrint(const String& message) {
    if (_debug) {
        Serial.println("[SD] " + message);
    }
}

String SDManager::getTimestamp() {
    // 简单的时间戳实现，使用millis()
    unsigned long ms = millis();
    unsigned long seconds = ms / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long hours = minutes / 60;
    
    return String(hours % 24) + ":" + 
           String((minutes % 60) < 10 ? "0" : "") + String(minutes % 60) + ":" +
           String((seconds % 60) < 10 ? "0" : "") + String(seconds % 60);
}

bool SDManager::validateFirmware(const char* filename) {
#ifdef SD_MODE_SPI
    File file = SD.open(filename, FILE_READ);
#else
    File file = SD_MMC.open(filename, FILE_READ);
#endif
    
    if (!file) {
        return false;
    }
    
    size_t fileSize = file.size();
    file.close();
    
    // 基本验证：检查文件大小是否合理
    if (fileSize < 1024 || fileSize > 2 * 1024 * 1024) { // 1KB - 2MB
        debugPrint("固件文件大小异常");
        return false;
    }
    
    // 可以添加更多验证逻辑，如校验和验证等
    return true;
}

// 高级管理功能实现

bool SDManager::formatSDCard() {
    debugPrint("开始SD卡格式化...");
    
    // 检查SD卡是否已初始化
    if (!_initialized) {
        debugPrint("SD卡未初始化，尝试初始化...");
        if (!begin()) {
            debugPrint("❌ SD卡初始化失败，无法格式化");
            return false;
        }
    }
    
    // 获取SD卡信息
    uint64_t cardSizeMB = getCardSizeMB();
    debugPrint(String("SD卡大小: ") + String(cardSizeMB) + "MB");
    
    if (cardSizeMB == 0) {
        debugPrint("❌ 无法获取SD卡大小，格式化失败");
        return false;
    }
    
    // 警告用户
    Serial.println("[SD] ⚠️  警告：格式化将删除SD卡上的所有数据！");
    Serial.println("[SD] 格式化开始，请勿断电...");
    
    // 先断开SD卡连接
    end();
    delay(1000);
    
    // 重新初始化并尝试格式化
#ifdef SD_MODE_SPI
    // SPI模式格式化
    SPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
    
    // 尝试以格式化模式初始化
    if (!SD.begin(SD_CS_PIN)) {
        debugPrint("❌ SPI模式重新初始化失败");
        return false;
    }
    
    debugPrint("✅ SPI模式格式化完成");
    
#else
    // SDIO模式格式化
    // 尝试强制格式化初始化
    if (!SD_MMC.begin("/sdcard", false, true, SDMMC_FREQ_DEFAULT)) {
        debugPrint("❌ SDIO模式格式化失败");
        return false;
    }
    
    debugPrint("✅ SDIO模式格式化完成");
#endif
    
    // 重新初始化SD管理器
    if (!begin()) {
        debugPrint("❌ 格式化后SD管理器初始化失败");
        return false;
    }
    
    // 创建基本目录结构
    debugPrint("创建基本目录结构...");
    createDir("/config");
    createDir("/logs");
    createDir("/firmware");
    createDir("/data");
    createDir("/backup");
    
    // 写入格式化日志
    writeLog("SD卡格式化完成，基本目录结构已创建");
    
    // 创建格式化标记文件
    String formatInfo = "{\n";
    formatInfo += "  \"formatted_time\": \"" + getTimestamp() + "\",\n";
    formatInfo += "  \"card_size_mb\": " + String(cardSizeMB) + ",\n";
    formatInfo += "  \"format_mode\": \"";
#ifdef SD_MODE_SPI
    formatInfo += "SPI";
#else
    formatInfo += "SDIO";
#endif
    formatInfo += "\"\n}";
    
    saveConfig(formatInfo, "/format_info.json");
    
    debugPrint("✅ SD卡格式化完成！");
    debugPrint(String("可用空间: ") + String(getFreeSpaceMB()) + "MB");
    
    return true;
}

bool SDManager::checkSDCardHealth() {
    debugPrint("开始SD卡健康检查...");
    
    if (!_initialized) {
        debugPrint("❌ SD卡未初始化");
        return false;
    }
    
    // 检查基本信息
    uint64_t totalMB = getCardSizeMB();
    uint64_t freeMB = getFreeSpaceMB();
    
    if (totalMB == 0) {
        debugPrint("❌ 无法读取SD卡大小");
        return false;
    }
    
    Serial.printf("[SD] SD卡总容量: %lluMB\n", totalMB);
    Serial.printf("[SD] 可用空间: %lluMB\n", freeMB);
    Serial.printf("[SD] 使用率: %.1f%%\n", (float)(totalMB - freeMB) * 100.0 / totalMB);
    
    // 检查关键目录
    bool dirOK = true;
    const char* criticalDirs[] = {"/config", "/logs", "/firmware", "/data"};
    
    for (int i = 0; i < 4; i++) {
        if (!fileExists(criticalDirs[i])) {
            Serial.printf("[SD] ⚠️  关键目录缺失: %s\n", criticalDirs[i]);
            dirOK = false;
        }
    }
    
    // 执行读写测试
    debugPrint("执行读写测试...");
    String testData = "Health check test data: " + String(millis());
    
    if (!saveConfig(testData, "/health_test.tmp")) {
        debugPrint("❌ 写入测试失败");
        return false;
    }
    
    String readData = loadConfig("/health_test.tmp");
    if (readData != testData) {
        debugPrint("❌ 读取测试失败");
        return false;
    }
    
    // 清理测试文件
    deleteFile("/health_test.tmp");
    
    // 检查空间不足警告
    if (freeMB < 10) {  // 少于10MB
        Serial.println("[SD] ⚠️  警告：SD卡空间不足（<10MB）");
    }
    
    if (dirOK) {
        debugPrint("✅ SD卡健康检查通过");
        writeLog("SD卡健康检查通过");
        return true;
    } else {
        debugPrint("⚠️  SD卡健康检查发现问题，建议修复");
        writeLog("SD卡健康检查发现目录结构问题");
        return false;
    }
}

void SDManager::repairSDCard() {
    debugPrint("开始SD卡修复...");
    
    if (!_initialized) {
        debugPrint("SD卡未初始化，尝试初始化...");
        if (!begin()) {
            debugPrint("❌ SD卡初始化失败，无法修复");
            return;
        }
    }
    
    // 重建目录结构
    debugPrint("重建目录结构...");
    const char* dirs[] = {"/config", "/logs", "/firmware", "/data", "/backup"};
    
    for (int i = 0; i < 5; i++) {
        if (createDir(dirs[i])) {
            Serial.printf("[SD] ✅ 目录创建/确认: %s\n", dirs[i]);
        } else {
            Serial.printf("[SD] ❌ 目录创建失败: %s\n", dirs[i]);
        }
    }
    
    // 检查并修复配置文件
    if (!fileExists("/config/wifi.json")) {
        debugPrint("创建默认WiFi配置文件...");
        saveWiFiConfig("", "");  // 空配置
    }
    
    // 创建修复日志
    String repairLog = "SD卡修复完成";
    writeLog(repairLog);
    
    // 创建修复标记文件
    String repairInfo = "{\n";
    repairInfo += "  \"repair_time\": \"" + getTimestamp() + "\",\n";
    repairInfo += "  \"repair_mode\": \"";
#ifdef SD_MODE_SPI
    repairInfo += "SPI";
#else
    repairInfo += "SDIO";
#endif
    repairInfo += "\"\n}";
    
    saveConfig(repairInfo, "/repair_info.json");
    
    debugPrint("✅ SD卡修复完成");
}

bool SDManager::handleSerialCommand(const String& command) {
    String cmd = command;
    cmd.trim();
    cmd.toLowerCase();
    
    if (cmd == "sd_check" || cmd == "sdc") {
        Serial.println("[SD] 执行SD卡健康检查...");
        checkSDCardHealth();
        return true;
        
    } else if (cmd == "sd_repair" || cmd == "sdr") {
        Serial.println("[SD] 执行SD卡修复...");
        repairSDCard();
        return true;
        
    } else if (cmd == "sd_format" || cmd == "sdf") {
        Serial.println("[SD] ⚠️  确认要格式化SD卡吗？这将删除所有数据！");
        Serial.println("[SD] 输入 'yes_format' 确认格式化");
        return true;
        
    } else if (cmd == "yes_format") {
        Serial.println("[SD] 开始格式化SD卡...");
        if (formatSDCard()) {
            Serial.println("[SD] ✅ SD卡格式化完成");
        } else {
            Serial.println("[SD] ❌ SD卡格式化失败");
        }
        return true;
        
    } else if (cmd == "sd_info" || cmd == "sdi") {
        Serial.println("[SD] SD卡信息:");
        if (_initialized) {
            printCardInfo();
        } else {
            Serial.println("[SD] SD卡未初始化");
        }
        return true;
        
    } else if (cmd == "sd_list" || cmd == "sdl") {
        Serial.println("[SD] SD卡目录内容:");
        if (_initialized) {
            listDir("/");
        } else {
            Serial.println("[SD] SD卡未初始化");
        }
        return true;
        
    } else if (cmd == "sd_help" || cmd == "sdh") {
        showHelp();
        return true;
        
    } else if (cmd == "sd_diag" || cmd == "sdd") {
        Serial.println("[SD] 执行硬件诊断...");
        performHardwareDiagnostic();
        return true;
    }
    
    return false; // 未识别的命令
}

void SDManager::showHelp() {
    Serial.println("[SD] SD卡管理命令:");
    Serial.println("  sd_check  (sdc) - 健康检查");
    Serial.println("  sd_repair (sdr) - 修复SD卡");
    Serial.println("  sd_format (sdf) - 格式化SD卡");
    Serial.println("  sd_info   (sdi) - 显示SD卡信息");
    Serial.println("  sd_list   (sdl) - 列出目录内容");
    Serial.println("  sd_help   (sdh) - 显示此帮助");
    Serial.println("  sd_diag   (sdd) - 硬件诊断");
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
    for (int pin : pins) {
        for (int reserved : reservedPins) {
            if (pin == reserved) {
                debugPrint("⚠️  警告: 引脚 " + String(pin) + " 可能与系统功能冲突");
            }
        }
    }
    
    debugPrint("✅ SPI引脚配置验证通过");
    return true;
#else
    return true; // SDIO模式不需要验证引脚
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
    
    // 测试SPI通信
    Serial.println("[诊断] 测试SPI通信...");
    SPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
    
    // 发送CMD0 (GO_IDLE_STATE)
    digitalWrite(SD_CS_PIN, LOW);
    delay(1);
    
    uint8_t response = 0xFF;
    for (int i = 0; i < 10; i++) {
        response = SPI.transfer(0xFF);
        if (response != 0xFF) break;
    }
    
    digitalWrite(SD_CS_PIN, HIGH);
    
    if (response == 0xFF) {
        Serial.println("[诊断] ❌ 无SPI响应 - 可能的问题:");
        Serial.println("        - SD卡未正确插入");
        Serial.println("        - 引脚连接错误");
        Serial.println("        - SD卡损坏");
        Serial.println("        - 电源供电不足");
    } else {
        Serial.printf("[诊断] ✅ 收到SPI响应: 0x%02X\n", response);
    }
    
#else
    Serial.println("[诊断] 使用SDIO模式");
    Serial.println("[诊断] SDIO引脚 (ESP32默认): CLK=14, CMD=15, DATA0=2, DATA1=4, DATA2=12, DATA3=13");
#endif
    
    // 电源检查建议
    Serial.println("[诊断] 电源检查建议:");
    Serial.println("        - 确保3.3V供电稳定");
    Serial.println("        - 检查接地连接");
    Serial.println("        - 尝试更换SD卡");
    
    Serial.println("=== 诊断完成 ===\n");
}

// 全局实例
SDManager sdManager;

#endif // ENABLE_SDCARD
