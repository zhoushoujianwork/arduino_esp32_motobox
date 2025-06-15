/*
 * ml307 AT Modem 驱动
 * 支持基础 AT 通信、网络注册等功能
 */

#ifndef ML307_AT_MODEM_H
#define ML307_AT_MODEM_H

#include <Arduino.h>
#include "config.h"

class Ml307AtModem {
public:
    Ml307AtModem(HardwareSerial& serial, int rxPin, int txPin);
    
    // 基础功能
    bool begin(uint32_t baudrate = 115200);
    void setDebug(bool debug);
    bool isNetworkReady();
    bool reset();
    
    // 网络信息
    String getLocalIP();
    int getCSQ();
    String getIMEI();
    String getICCID();
    String getModuleName();
    String getCarrierName();
    
    // AT 命令
    bool sendAT(const String& cmd, const String& expected = "OK", uint32_t timeout = 1000);
    bool sendATWithRetry(const String& cmd, const String& expected = "OK", int maxRetry = 5, uint32_t timeout = 2000);
    String sendATWithResponse(const String& cmd, uint32_t timeout = 1000);
    
    // 状态检查
    bool isReady();

private:
    HardwareSerial& _serial;
    int _rxPin;
    int _txPin;
    bool _debug;
    static const int RX_BUFFER_SIZE = 1024;
    char _rxBuffer[RX_BUFFER_SIZE];
    
    void flushInput();
    String waitResponse(uint32_t timeout = 1000);
    bool expectResponse(const String& expected, uint32_t timeout = 1000);
    void debugPrint(const String& msg);

    bool initModem();
    bool tryBaudrate(uint32_t baudrate);
};

extern Ml307AtModem ml307;

#endif // ml307_AT_MODEM_H 