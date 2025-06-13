/*
 * ML307 AT Modem 驱动（适用于 Arduino/ESP32 框架）
 * 支持基础 AT 通信、网络注册、GPRS拨号、MQTT等功能
 * 适配 PubSubClient 等 MQTT 库
 * 需根据实际串口引脚初始化
 */

#ifndef ML307_AT_MODEM_H
#define ML307_AT_MODEM_H

#include <Arduino.h>
#include <Client.h> // 兼容 PubSubClient
#include "config.h"

#define DEFAULT_COMMAND_TIMEOUT 3000
#define DEFAULT_BAUD_RATE 115200
#define DEFAULT_UART_NUM UART_NUM_1

struct AtArgumentValue {
    enum class Type {
        String,
        Int,
        Double
    };
    Type type;
    std::string string_value;
    int int_value;
    double double_value;
};

class Ml307AtModem : public Client {
private:
    HardwareSerial& _serial;
    uint32_t _baudrate;
    int8_t _rxPin;
    int8_t _txPin;
    int8_t _resetPin;
    bool _connected = false;
    
    static const int RX_BUFFER_SIZE = 4096;  // 增大缓存
    uint8_t _rxBuffer[RX_BUFFER_SIZE];
    size_t _rxBufferIndex = 0;
    size_t _rxBufferLength = 0;
    String _responseBuffer;  // 新增响应缓存
    
    String sendRaw(const String& cmd, uint32_t timeout = 1000);
    void flushInput();
    bool connectGPRS(const String& apn);
    void processRxData();
    String extractIP(const String& response);
    int parseCSQ(const String& response);
    bool autoBaudrate();

public:
    /**
     * 构造函数，指定串口、波特率、引脚
     */
    Ml307AtModem(HardwareSerial& serial, uint32_t baudrate, int8_t rxPin, int8_t txPin, int8_t resetPin = -1);

    void begin();

    /**
     * 获取本地IP地址
     */
    String getLocalIP();

    /**
     * 获取信号强度
     */
    int getCSQ();

    /**
     * 获取IMEI
     */
    String getIMEI();

    /**
     * 获取ICCID
     */
    String getICCID();

    /**
     * 发送AT命令
     */
    bool sendAT(const String& cmd, const String& expect = "OK", uint32_t timeout = 1000);

    /**
     * 等待网络连接
     */
    bool waitForNetwork(uint32_t timeout = 30000);

    /**
     * 重置调制解调器
     */
    bool resetModem();

    /**
     * 检查网络状态
     */
    bool checkNetworkStatus();

    // isReady
    bool isReady();

    // --- 下面为Client接口，兼容PubSubClient ---
    virtual int connect(IPAddress ip, uint16_t port);
    virtual int connect(const char *host, uint16_t port);
    virtual size_t write(uint8_t);
    virtual size_t write(const uint8_t *buf, size_t size);
    virtual int available();
    virtual int read();
    virtual int read(uint8_t *buf, size_t size);
    virtual int peek();
    virtual void flush();
    virtual void stop();
    virtual uint8_t connected();
    virtual operator bool();

    virtual ~Ml307AtModem() {
        stop();
    }
};
#ifdef ENABLE_GSM
extern Ml307AtModem ml370;
#endif

#endif
