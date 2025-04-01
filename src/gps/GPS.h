#ifndef GPS_H
#define GPS_H

#include <SoftwareSerial.h>
#include <TinyGPS++.h>
#include "device.h"
#include "config.h"
#include <ArduinoJson.h>

// 将gps_data_t结构体转换为JSON字符串
String gps_data_to_json(gps_data_t gps_data);

// GPS命令类型枚举
enum GpsCommandType {
    CMD_BAUDRATE,    // 波特率设置命令
    CMD_MODE,        // 工作模式设置命令
    CMD_UPDATE_RATE, // 数据更新率设置命令
    CMD_SENTENCE,    // NMEA语句输出设置命令
    CMD_QUERY_INFO   // 查询产品信息命令
};

class GpsCommand {
public:
    // 构建波特率设置命令
    static String buildBaudrateCmd(int baudRate);
    
    // 构建工作模式设置命令
    static String buildModeCmd(int mode);
    
    // 构建更新率设置命令
    static String buildUpdateRateCmd(int ms);
    
    // 构建NMEA语句输出设置命令
    static String buildNmeaSentenceCmd(bool gga, bool gll, bool gsa, bool gsv, 
                                       bool rmc, bool vtg, bool zda);
    
    // 计算NMEA命令校验和
    static String calculateChecksum(const String& cmd);
};

class GPS
{
public:
    GPS(int rxPin, int txPin);
    void begin();
    void loop();
    bool setGpsHz(int hz);
    // 打印原始数据
    void printRawData();
    
    // 根据卫星数量自动调整GPS更新频率
    bool autoAdjustUpdateRate();
    
    // 打印GPS数据接收统计信息
    void printGpsStats();
    

private:
    // 配置GPS工作模式，启用双模式和完整NMEA输出
    void configGpsMode();
    
    // 从TinyGPS++更新设备GPS数据
    void updateGpsData();
    
    // 检查 GPS 天线 状态
    bool checkGpsStatus();
    SoftwareSerial gpsSerial;
    TinyGPSPlus gps;
    int txmsgCount;


    // 异步初始化标志
    static bool _initInProgress;

    void asyncInit();
    
    // 发送GPS命令并等待响应
    bool sendGpsCommand(const String& cmd, int retries = 3, int retryDelay = 100);
    
    // 旧的配置方法，保留以兼容现有代码
    void configGps(const char *configCmd);
    
    int _rxPin;
    int _txPin;
    int _currentHz;  // 当前GPS更新频率
    
    // GPS数据接收统计相关
    unsigned long _lastStatTime;
    unsigned long _validSentences;
    unsigned long _invalidSentences;
    unsigned long _lastLocationUpdateTime;
};

extern GPS gps;

#endif // GPS_H