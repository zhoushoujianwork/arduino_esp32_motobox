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
    bool recordGPSData(double latitude, double longitude, double altitude = 0.0, float speed = 0.0, int satellites = 0);

    // 串口命令处理
    bool handleSerialCommand(const String& command);

private:
    bool _initialized;

    // 内部方法
    bool createDirectoryStructure();
    bool createDirectory(const char* path);
    
    // 工具方法
    String getDeviceID();
    String getCurrentTimestamp();
    String getCurrentDateString();
    void debugPrint(const String& message);
};

#endif // SDMANAGER_H
