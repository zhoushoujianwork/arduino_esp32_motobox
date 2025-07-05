/*
 * Air780EG 4G+GNSS Modem 驱动
 * 支持4G Cat.1通信、GNSS定位、LBS定位等功能
 */

#ifndef AIR780EG_MODEM_H
#define AIR780EG_MODEM_H

#include <Arduino.h>
#include "config.h"
#include "../gps/GPSData.h"
#include "../gps/LBSData.h"
#include <mutex>

// GNSS数据结构
struct GNSSData {
    bool valid;
    float latitude;
    float longitude;
    float altitude;
    float speed;
    float course;
    int satellites;
    float hdop;
    String timestamp;
    String fix_mode;  // 2D/3D
    
    GNSSData() : valid(false), latitude(0), longitude(0), altitude(0), 
                 speed(0), course(0), satellites(0), hdop(0) {}
};

class Air780EGModem {
public:
    Air780EGModem(HardwareSerial& serial, int rxPin, int txPin, int enPin);
    
    // 基础功能
    bool begin(uint32_t baudrate = 115200);
    void setDebug(bool debug);
    bool isNetworkReady() { return _isNetworkReady; }
    bool isNetworkReadyCheck();
    void loop();     // 后台初始化处理
    bool isFullyInitialized();      // 检查是否完全初始化完成
    bool reset();
    bool powerOn();
    bool powerOff();
    
    // 网络信息
    String getLocalIP();
    int getCSQ();
    String getIMEI();
    String getICCID();
    String getModuleName();
    String getCarrierName();
    String getNetworkType();
    
    // LBS 功能
    bool enableLBS(bool enable);
    bool isLBSEnabled();
    String getLBSRawData();
    bool updateLBSData();
    bool isLBSLoading() { return _isLBSLoading; }
    lbs_data_t getLBSData();
    bool isLBSDataValid();
    
    // GNSS 功能
    bool enableGNSS(bool enable);
    bool isGNSSEnabled();
    bool isGNSSFixed();           // 检查是否已定位
    bool isGNSSWaitingFix();      // 检查是否正在等待定位
    String getGNSSInfo();         // 获取GNSS详细信息
    bool updateGNSSData();
    GNSSData getGNSSData();
    bool isGNSSDataValid();
    bool setGNSSUpdateRate(int hz);  // 设置更新频率 1/2/5/10Hz
    String getGNSSRawData();
    
    // GPS数据转换 (兼容现有接口)
    gps_data_t getGPSData();  // 将GNSS数据转换为GPS格式
    bool isGPSDataValid();
    
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

    // GNSS URC处理
    void processURC();                          // 处理未请求的结果代码
    bool handleGNSSURC(const String& urc);      // 处理GNSS自动上报数据
    
    // 调试功能
    void debugNetworkInfo();
    void debugGNSSInfo();
    void debugLBSConfig();
    void testATCommand();  // 添加AT命令测试方法

private:
    HardwareSerial& _serial;
    int _rxPin;
    int _txPin;
    int _enPin;
    bool _debug;
    bool _lbsEnabled;
    bool _gnssEnabled;
    
    // 网络状态
    bool _isNetworkReady;
    unsigned long _lastNetworkReadyCheckTime;
    
    // 后台初始化状态
    enum InitState {
        INIT_IDLE,
        INIT_WAITING_NETWORK,
        INIT_ENABLING_GNSS,
        INIT_COMPLETED
    };
    InitState _initState;
    unsigned long _initStartTime;
    unsigned long _lastInitCheck;
    
    // 看门狗和安全机制
    unsigned long _lastLoopTime;
    unsigned long _loopTimeoutCount;
    static const unsigned long LOOP_TIMEOUT_MS = 30000; // 30秒超时
    static const unsigned long MAX_TIMEOUT_COUNT = 3;   // 最大超时次数
    
    // LBS相关私有成员
    unsigned long _lastLBSUpdate;
    bool _isLBSLoading;
    String _lastLBSLocation;
    unsigned long _lastCMEErrorTime;
    int _cmeErrorCount;
    unsigned long _backoffDelay;
    
    // GNSS相关私有成员
    unsigned long _lastGNSSUpdate;
    GNSSData _gnssData;
    String _lastGNSSRaw;
    int _gnssUpdateRate;  // Hz
    
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
    bool parseLBSResponse(const String& response);
    
    // GNSS解析方法
    bool parseGNSSResponse(const String& response);
    bool parseNMEA(const String& nmea);
    
    // 互斥锁
    std::mutex _atMutex;
};

extern Air780EGModem air780eg_modem;

#endif // AIR780EG_MODEM_H
