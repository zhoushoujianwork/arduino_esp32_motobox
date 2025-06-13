#ifndef LED_H
#define LED_H

#include <Arduino.h>

class LED
{
public:
    enum Mode
    {
        OFF,       // 关闭
        ON,        // 常亮
        BLINK_SINGLE, // 单闪
        BLINK_DUAL, // 双闪
        BLINK_5_SECONDS // 5 秒钟闪烁一次
    };

    LED(uint8_t pin);
    void setMode(Mode mode);
    void begin();
    void loop();

private:
    uint8_t _pin;
    Mode _mode;
    Mode _lastMode; // 添加上一次模式记录
    unsigned long _lastToggle;
    bool _state;
    uint8_t _blinkCount;
    
    // 添加辅助函数声明
    const char* modeToString(Mode mode);

    static const unsigned long BLINK_INTERVAL = 100;    // 闪烁间隔(ms)
    static const unsigned long PATTERN_INTERVAL = 1000; // 整体模式重复间隔
};  

#ifdef LED_PIN
extern LED led;
#endif

#endif