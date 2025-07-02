#ifndef EXTERNAL_POWER_H
#define EXTERNAL_POWER_H

#include <Arduino.h>

class ExternalPower
{
public:
    ExternalPower(int pin);
    void begin();
    void loop();
    
    // 获取外部电源状态
    bool isConnected();
    
    // 添加调试控制
    void setDebug(bool debug) { _debug = debug; }

private:
    const int _pin;
    bool _is_connected;      // 外部电源连接状态缓存
    bool _last_state;        // 上一次状态，用于检测变化
    unsigned long _last_check_time; // 上次检查时间
    static const unsigned long CHECK_INTERVAL = 100; // 检查间隔(ms)
    
    // 防抖相关
    static const unsigned long DEBOUNCE_DELAY = 50; // 防抖延时(ms)
    unsigned long _last_change_time;
    bool _raw_state;
    
    // 调试相关
    bool _debug;
    void debugPrint(const String& message);
    
    // 状态检测
    bool readRawState();
    void updateState();
};

#ifdef RTC_INT_PIN
extern ExternalPower externalPower;
#endif

#endif
