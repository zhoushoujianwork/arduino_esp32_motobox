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
    bool valid;                // 数据是否有效
    float latitude;           // 纬度
    float longitude;          // 经度
    float altitude;           // 海拔
    float speed;             // 速度
    float course;            // 航向
    int satellites;          // 卫星数量
    float hdop;              // 水平精度因子
    String timestamp;        // 时间戳
    String fix_mode;         // 定位模式(2D/3D)
    
    GNSSData() : valid(false), latitude(0), longitude(0), altitude(0), 
                 speed(0), course(0), satellites(0), hdop(0) {}
};

class Air780EGModem {
public:
    Air780EGModem(HardwareSerial& serial, int rxPin, int txPin, int enPin);
    
    // 基础功能
    bool begin(uint32_t baudrate = 115200);
    bool isNetworkReady() { return _isNetworkReady; }
    bool isNetworkReadyCheck();
    void loop();                    // 后台任务处理
    bool isFullyInitialized();      // 检查初始化状态
    bool reset();                   // 重置模块
    bool powerOn();                 // 开启模块
    bool powerOff();                // 关闭模块
    
    // 网络信息
    String getLocalIP();            // 获取本地IP
    int getCSQ();                   // 获取信号强度
    String getIMEI();               // 获取IMEI
    String getICCID();              // 获取ICCID
    String getModuleName();         // 获取模块名称
    String getCarrierName();        // 获取运营商名称
    String getNetworkType();        // 获取网络类型
    
    // LBS功能
    bool enableLBS(bool enable);     // 启用/禁用LBS
    bool isLBSEnabled();            // 检查LBS是否启用
    bool updateLBSData();           // 更新LBS数据
    bool isLBSLoading() { return _isLBSLoading; }
    lbs_data_t getLBSData();        // 获取LBS数据
    bool isLBSDataValid();          // 检查LBS数据有效性
    
    // GNSS功能
    bool enableGNSS(bool enable);    // 启用/禁用GNSS
    bool enableGNSSAutoReport(bool enable);  // 控制GNSS自动上报
    bool isGNSSEnabled();           // 检查GNSS是否启用
    bool isGNSSFixed();             // 检查是否已定位
    bool isGNSSWaitingFix();        // 检查是否等待定位
    bool updateGNSSData();          // 更新GNSS数据
    GNSSData getGNSSData();         // 获取GNSS数据
    bool isGNSSDataValid();         // 检查GNSS数据有效性
    bool setGNSSUpdateRate(int hz); // 设置更新频率
    
    // GPS数据转换
    gps_data_t getGPSData();        // GNSS数据转GPS格式
    bool isGPSDataValid();          // 检查GPS数据有效性
    
    // AT命令处理
    bool sendAT(const String& cmd, const String& expected = "OK", uint32_t timeout = 1000);
    bool sendATWithRetry(const String& cmd, const String& expected = "OK", int maxRetry = 5, uint32_t timeout = 2000);
    String sendATWithResponse(const String& cmd, uint32_t timeout = 1000);
    
    // MQTT回调管理
    void setMQTTMessageHandler(void (*callback)(String topic, String payload));
    
    // 线程安全的AT命令
    bool sendATThreadSafe(const String& cmd, const String& expected = "OK", uint32_t timeout = 1000);
    String sendATWithResponseThreadSafe(const String& cmd, uint32_t timeout = 1000);
    bool sendATWithRetryThreadSafe(const String &cmd, const String &expected = "OK", int maxRetry = 3, uint32_t timeout = 2000);

private:
    HardwareSerial& _serial;
    int _rxPin;
    int _txPin;
    int _enPin;
    bool _lbsEnabled;
    bool _gnssEnabled;
    bool _isNetworkReady;
    
    // 初始化状态
    enum InitState {
        INIT_IDLE,
        INIT_WAITING_NETWORK,
        INIT_ENABLING_GNSS,
        INIT_COMPLETED
    };
    InitState _initState;
    unsigned long _initStartTime;
    unsigned long _lastInitCheck;
    unsigned long _lastNetworkReadyCheckTime;
    
    // LBS相关
    unsigned long _lastLBSUpdate;
    bool _isLBSLoading;
    String _lastLBSLocation;
    lbs_data_t _lbsData;
    unsigned long _lastCMEErrorTime;
    int _cmeErrorCount;
    unsigned long _backoffDelay;
    
    // MQTT回调
    void (*_mqttMessageHandler)(String topic, String payload);
    
    // 私有方法
    void flushInput();
    bool initModem();
    bool tryBaudrate(uint32_t baudrate);
    bool parseGNSSResponse(const String& response);
    String waitResponse(uint32_t timeout = 1000);
    bool expectResponse(const String& expected, uint32_t timeout = 1000);
    
    // 互斥锁
    std::mutex _atMutex;
};

extern Air780EGModem air780eg_modem;

#endif // AIR780EG_MODEM_H
