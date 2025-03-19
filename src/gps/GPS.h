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

private:
    // 检查 GPS 天线 状态
    bool checkGpsStatus();
    bool setGpsBaudRate(int baudRate);
    SoftwareSerial gpsSerial;
    TinyGPSPlus gps;
    int txmsgCount;
    void setMode1();
    void setMode2();
    
    // 发送GPS命令并等待响应
    bool sendGpsCommand(const String& cmd, int retries = 3, int retryDelay = 100);
    
    // 旧的配置方法，保留以兼容现有代码
    void configGps(const char *configCmd);
    
    int _rxPin;
    int _txPin;
    int _currentHz;  // 当前GPS更新频率
};

extern GPS gps;

#endif // GPS_H