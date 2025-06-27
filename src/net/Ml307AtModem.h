/*
 * ml307 AT Modem 驱动
 * 支持基础 AT 通信、网络注册、LBS定位等功能
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
    bool begin(uint32_t baudrate = 115200);
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
    
    // LBS 功能 - 主要定位方式
    bool enableLBS(bool enable);
    bool isLBSEnabled();
    String getLBSRawData();  // 返回原始响应字符串
    bool updateLBSData();
    bool isLBSLoading() { return _isLBSLoading; }
    lbs_data_t getLBSData();  // 获取解析后的LBS数据
    bool isLBSDataValid();    // 检查LBS数据是否有效
    
    // AT 命令
    bool sendAT(const String& cmd, const String& expected = "OK", uint32_t timeout = 1000);
    bool sendATWithRetry(const String& cmd, const String& expected = "OK", int maxRetry = 5, uint32_t timeout = 2000);
    String sendATWithResponse(const String& cmd, uint32_t timeout = 1000);
    
    // 状态检查
    bool isReady();

    // 线程安全的AT命令方法
    bool sendATThreadSafe(const String& cmd, const String& expected = "OK", uint32_t timeout = 1000);
    String sendATWithResponseThreadSafe(const String& cmd, uint32_t timeout = 1000);
    bool sendATWithRetryThreadSafe(const String &cmd, const String &expected = "OK", int maxRetry = 3, uint32_t timeout = 2000);

    // 调试功能
    void debugLBSConfig();  // LBS配置调试函数

private:
    HardwareSerial& _serial;
    int _rxPin;
    int _txPin;
    bool _debug;
    bool _lbsEnabled;
    
    // 网络状态
    bool _isNetworkReady;
    unsigned long _lastNetworkReadyCheckTime;
    
    // LBS相关私有成员
    unsigned long _lastLBSUpdate;         // 上次LBS更新时间
    bool _isLBSLoading;
    String _lastLBSLocation;              // 存储原始LBS响应
    unsigned long _lastCMEErrorTime;      // 上次CME错误时间
    int _cmeErrorCount;                   // CME错误计数
    unsigned long _backoffDelay;           // 退避延迟时间
    
    // 私有方法
    void flushInput();
    unsigned long calculateBackoffDelay();
    bool shouldSkipLBSRequest();
    void handleCMEError();
    void resetCMEErrorCount();

    String waitResponse(uint32_t timeout = 1000);
    bool expectResponse(const String& expected, uint32_t timeout = 1000);
    void debugPrint(const String& msg);
    bool initModem();
    bool tryBaudrate(uint32_t baudrate);
    
    // LBS解析方法
    bool parseLBSResponse(const String& response);  // 解析LBS响应
    
    // 互斥锁
    std::mutex _atMutex;
};

extern Ml307AtModem ml307_at;

#endif // ML307_AT_MODEM_H 