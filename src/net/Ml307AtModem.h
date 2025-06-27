/*
 * ml307 AT Modem 驱动
 * 支持基础 AT 通信、网络注册、GNSS定位等功能
 */

#ifndef ML307_AT_MODEM_H
#define ML307_AT_MODEM_H

#include <Arduino.h>
#include "config.h"
#include "../gps/GPSData.h"
#include "../gps/LBSData.h"
#include <mutex>


class Ml307AtModem {
public:
    Ml307AtModem(HardwareSerial& serial, int rxPin, int txPin);
    
    // 基础功能
    bool begin(uint32_t baudrate = 921600);
    void setDebug(bool debug);
    bool isNetworkReady() { return _isNetworkReady; }
    bool isNetworkReadyCheck();

    bool reset();
    
    // 网络信息
    String getLocalIP();
    int getCSQ();
    String getIMEI();
    String getICCID();
    String getModuleName();
    String getCarrierName();
    
    // GNSS 功能
    bool gnssPower(bool on);
    bool gnssInit();
    bool enableGNSS(bool enable);
    bool isGNSSEnabled();
    bool isGNSSReady();
    gps_data_t getGNSSData();
    String getGNSSInfo();
    bool updateGNSSData();
    
    // LBS 功能 - 简化版本
    bool enableLBS(bool enable);
    bool isLBSEnabled();
    String getLBSRawData();  // 只返回原始响应字符串
    bool updateLBSData();
    bool isLBSLoading() { return _isLBSLoading; }
    
    // AT 命令
    bool sendAT(const String& cmd, const String& expected = "OK", uint32_t timeout = 1000);
    bool sendATWithRetry(const String& cmd, const String& expected = "OK", int maxRetry = 5, uint32_t timeout = 2000);
    String sendATWithResponse(const String& cmd, uint32_t timeout = 1000);
    
    // 状态检查
    bool isReady();

    // 添加线程安全的AT命令方法
    bool sendATThreadSafe(const String& cmd, const String& expected = "OK", uint32_t timeout = 1000);
    String sendATWithResponseThreadSafe(const String& cmd, uint32_t timeout = 1000);
    bool sendATWithRetryThreadSafe(const String &cmd, const String &expected = "OK", int maxRetry = 3, uint32_t timeout = 2000);

    // 在public部分添加
    void debugLBSConfig();  // LBS配置调试函数

private:
    HardwareSerial& _serial;
    int _rxPin;
    int _txPin;
    bool _debug;
    bool _gnssEnabled;
    bool _lbsEnabled;
    gps_data_t _gnssData;

    bool _isNetworkReady;
    unsigned long _lastNetworkReadyCheckTime;
    
    // LBS相关私有成员 - 简化
    unsigned long _lastLBSUpdate;         // 上次LBS更新时间
    bool _isLBSLoading;
    String _lbsRawResponse;               // 存储原始LBS响应
    
    // 其他私有成员
    unsigned long _lastGNSSUpdate;
    static const int RX_BUFFER_SIZE = 1024;
    char _rxBuffer[RX_BUFFER_SIZE];
    
    // 私有方法
    void flushInput();
    String waitResponse(uint32_t timeout = 1000);
    String _lastLBSLocation;
    bool expectResponse(const String& expected, uint32_t timeout = 1000);
    void debugPrint(const String& msg);

    bool initModem();
    bool tryBaudrate(uint32_t baudrate);
    void processURC(const String& urc);

    
    // GNSS 内部方法
    bool parseGNSSInfo(const String& response);
    void resetGNSSData();

    
    std::mutex _atMutex;  // AT命令互斥锁
};

extern Ml307AtModem ml307_at;

#endif // ML307_AT_MODEM_H 