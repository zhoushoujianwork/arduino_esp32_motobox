/*
 * SD卡管理类
 * 支持SDIO和SPI两种模式，根据config.h配置自动选择
 */

#ifndef SDMANAGER_H
#define SDMANAGER_H

#include <Arduino.h>
#include "config.h"

// 根据配置选择对应的库
#ifdef SD_MODE_SPI
#include "SD.h"
#include "SPI.h"
#else
#include "SD_MMC.h"
// esp32 默认引脚,CLK:GPIO14,CMD:GPIO15,DATA0:GPIO2,DATA1:GPIO4,DATA2:GPIO12,DATA3:GPIO13
#define SD_SCK_PIN 14
#define SD_CS_PIN 15
#define SD_MOSI_PIN 13
#define SD_MISO_PIN 12 
#endif

#include <ArduinoJson.h>

class SDManager
{
public:
    SDManager();
    
    /**
     * @brief 初始化SD卡
     * @return true 初始化成功，false 初始化失败
     */
    bool begin();
    
    /**
     * @brief 结束SD卡操作，释放资源
     */
    void end();
    
    /**
     * @brief 检查SD卡是否已初始化
     * @return true 已初始化，false 未初始化
     */
    bool isInitialized() const { return _initialized; }
    
    /**
     * @brief 获取SD卡信息
     */
    void printCardInfo();
    
    /**
     * @brief 获取SD卡大小（MB）
     * @return SD卡大小，单位MB
     */
    uint64_t getCardSizeMB() const;
    
    /**
     * @brief 获取SD卡剩余空间（MB）
     * @return 剩余空间，单位MB
     */
    uint64_t getFreeSpaceMB() const;
    
    // 硬件诊断方法
    /**
     * @brief 验证SPI引脚配置
     * @return true 配置有效，false 配置无效
     */
    bool validateSPIPins();
    
    /**
     * @brief 验证SD卡基本操作
     * @return true 操作正常，false 操作失败
     */
    bool verifySDCardOperation();
    
    /**
     * @brief 执行硬件诊断
     */
    void performHardwareDiagnostic();
    
    // 文件操作
    /**
     * @brief 检查文件是否存在
     * @param filename 文件名
     * @return true 存在，false 不存在
     */
    bool fileExists(const char* filename);
    
    /**
     * @brief 删除文件
     * @param filename 文件名
     * @return true 删除成功，false 删除失败
     */
    bool deleteFile(const char* filename);
    
    /**
     * @brief 创建目录
     * @param dirname 目录名
     * @return true 创建成功，false 创建失败
     */
    bool createDir(const char* dirname);
    
    /**
     * @brief 列出目录内容
     * @param dirname 目录名
     */
    void listDir(const char* dirname);
    
    // 固件升级相关
    /**
     * @brief 检查是否有固件更新文件
     * @return true 有更新文件，false 无更新文件
     */
    bool hasFirmwareUpdate();
    
    /**
     * @brief 执行固件升级
     * @param filename 固件文件名
     * @return true 升级成功，false 升级失败
     */
    bool performFirmwareUpdate(const char* filename = "/firmware.bin");
    
    /**
     * @brief 备份当前固件
     * @param filename 备份文件名
     * @return true 备份成功，false 备份失败
     */
    bool backupCurrentFirmware(const char* filename = "/firmware_backup.bin");
    
    // 配置文件管理
    /**
     * @brief 保存配置到SD卡
     * @param config 配置内容
     * @param filename 配置文件名
     * @return true 保存成功，false 保存失败
     */
    bool saveConfig(const String& config, const char* filename);
    
    /**
     * @brief 从SD卡加载配置
     * @param filename 配置文件名
     * @return 配置内容，空字符串表示加载失败
     */
    String loadConfig(const char* filename);
    
    /**
     * @brief 保存WiFi配置
     * @param ssid WiFi名称
     * @param password WiFi密码
     * @return true 保存成功，false 保存失败
     */
    bool saveWiFiConfig(const String& ssid, const String& password);
    
    /**
     * @brief 加载WiFi配置
     * @param ssid 输出WiFi名称
     * @param password 输出WiFi密码
     * @return true 加载成功，false 加载失败
     */
    bool loadWiFiConfig(String& ssid, String& password);
    
    // 日志管理
    /**
     * @brief 写入日志
     * @param message 日志消息
     * @param filename 日志文件名
     * @return true 写入成功，false 写入失败
     */
    bool writeLog(const String& message, const char* filename = "/system.log");
    
    /**
     * @brief 读取日志文件
     * @param filename 日志文件名
     * @param maxLines 最大读取行数，0表示读取全部
     * @return 日志内容
     */
    String readLog(const char* filename = "/system.log", int maxLines = 0);
    
    /**
     * @brief 清空日志文件
     * @param filename 日志文件名
     * @return true 清空成功，false 清空失败
     */
    bool clearLog(const char* filename = "/system.log");
    
    /**
     * @brief 设置调试模式
     * @param debug 是否开启调试
     */
    void setDebug(bool debug) { _debug = debug; }
    
    /**
     * @brief 获取当前时间戳字符串（公共方法）
     * @return 时间戳字符串
     */
    String getTimestamp();
    
    /**
     * @brief 为深度睡眠准备SD卡
     */
    void prepareForSleep();
    
    /**
     * @brief 从深度睡眠恢复SD卡
     */
    void restoreFromSleep();
    
    // 高级管理功能
    /**
     * @brief 格式化SD卡
     * @return true 格式化成功，false 格式化失败
     */
    bool formatSDCard();
    
    /**
     * @brief 检查SD卡健康状态
     * @return true 健康，false 有问题
     */
    bool checkSDCardHealth();
    
    /**
     * @brief 修复SD卡（重建目录结构）
     */
    void repairSDCard();
    
    /**
     * @brief 处理串口命令
     * @param command 命令字符串
     * @return true 命令已处理，false 未识别的命令
     */
    bool handleSerialCommand(const String& command);
    
    /**
     * @brief 显示帮助信息
     */
    void showHelp();

private:
    bool _initialized;
    bool _debug;
    
    /**
     * @brief 调试输出
     * @param message 调试消息
     */
    void debugPrint(const String& message);
    
    /**
     * @brief 验证固件文件
     * @param filename 固件文件名
     * @return true 验证通过，false 验证失败
     */
    bool validateFirmware(const char* filename);
};

#ifdef ENABLE_SDCARD
extern SDManager sdManager;
#endif

#endif // SDMANAGER_H
