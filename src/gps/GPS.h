#ifndef GPS_H
#define GPS_H


#include <SoftwareSerial.h>

class GPS
{
public:
    GPS(int rxPin, int txPin);
    void begin();
    void loop();
    void printRawData();
    int changeHz();
    bool setBaudRate(int baudRate);
    bool setHz(int hz);
private:
    SoftwareSerial gpsSerial;
    int _rxPin;
    int _txPin;
    int _hz;
    // 构建波特率设置命令
    static String buildBaudrateCmd(int baudRate);
    // 构建工作模式设置命令
    static String buildModeCmd(int mode);
    // 构建更新率设置命令
    static String buildUpdateRateCmd(int ms);
    // 计算NMEA命令校验和
    static String calculateChecksum(const String& cmd);
    // 发送GPS命令
    bool sendGpsCommand(const String& cmd, int retries, int retryDelay);
};

extern GPS gps;

#endif // GPS_H