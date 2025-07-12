#ifndef SDMANAGER_H
#define SDMANAGER_H

#include <Arduino.h>
#include <SPI.h>

#ifdef SD_MODE_SPI
#include <SD.h>
#else
#include <SD_MMC.h>
#endif

#include <esp_system.h>

#ifndef FIRMWARE_VERSION
#define FIRMWARE_VERSION "2.3.0"
#endif

#include "Air780EG.h"
#include "Air780EGGNSS.h"

class SDManager {
public:
    SDManager();
    ~SDManager();

    // 基本操作
    bool begin();
    void end();
    bool isInitialized();

    // 空间信息
    uint64_t getTotalSpaceMB();
    uint64_t getFreeSpaceMB();

    // 核心功能
    bool saveDeviceInfo();
    bool recordGPSData(gnss_data_t &gnss_data);
    bool finishGPSSession();

    // 串口命令处理
    bool handleSerialCommand(const String& command);

    // ========== 简单语音文件支持 ==========
    /**
     * @brief 检查SD卡上是否存在自定义欢迎语音文件
     * @return 是否存在 /voice/welcome.wav
     */
    bool hasCustomWelcomeVoice();
    /**
     * @brief 获取自定义欢迎语音文件路径
     * @return 文件路径 "/voice/welcome.wav"
     */
    String getCustomWelcomeVoicePath();
    /**
     * @brief 检查语音文件是否有效（WAV格式）
     * @return 是否有效
     */
    bool isValidWelcomeVoiceFile();

private:
    bool _initialized;

    // 内部方法
    bool createDirectoryStructure();
    bool createDirectory(const char* path);
    bool ensureGPSDirectoryExists();
    bool directoryExists(const char* path);
    
    // 工具方法
    String getDeviceID();
    String getCurrentTimestamp();
    String getCurrentDateString();
    String getCurrentTimeString();
    String generateGPSSessionFilename();
    int getBootCount();
    void debugPrint(const String& message);
};

#endif // SDMANAGER_H
