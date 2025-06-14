/*
 * ML370 AT Modem 驱动
 * 支持基础 AT 通信、网络注册等功能
 */

#ifndef ML370_AT_MODEM_H
#define ML370_AT_MODEM_H

#include <Arduino.h>

class Ml370AtModem {
public:
    Ml370AtModem(HardwareSerial& serial = Serial2);
    
    // 基础功能
    bool begin(uint32_t baudrate = 115200);
    void setDebug(bool debug);
    bool waitForNetwork(uint32_t timeout = 30000);
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
    String sendATWithResponse(const String& cmd, uint32_t timeout = 1000);
    
    // 状态检查
    bool isReady();

private:
    HardwareSerial& _serial;
    bool _debug;
    static const int RX_BUFFER_SIZE = 1024;
    char _rxBuffer[RX_BUFFER_SIZE];
    
    void flushInput();
    String waitResponse(uint32_t timeout = 1000);
    bool expectResponse(const String& expected, uint32_t timeout = 1000);
    void debugPrint(const String& msg);
};

extern Ml370AtModem ml370;

#endif // ML370_AT_MODEM_H 