#ifndef LED_H
#define LED_H

#include <Arduino.h>

class LED
{
public:
    enum Mode {
        OFF,
        ON,
        BLINK_SINGLE,
        BLINK_DUAL,
        BLINK_5_SECONDS,
        INIT_BLINK
    };

    LED(uint8_t pin);
    void begin();
    void loop();
    void setMode(Mode mode);
    const char* modeToString(Mode mode);
    void initBlink(uint8_t times);

private:
    static const unsigned long BLINK_INTERVAL = 100;    // 闪烁间隔(ms)
    static const unsigned long PATTERN_INTERVAL = 1000; // 整体模式重复间隔
    static const unsigned long INIT_BLINK_INTERVAL = 200; // 200ms闪烁间隔

    uint8_t _pin;
    Mode _mode;
    Mode _lastMode; // 添加上一次模式记录
    unsigned long _lastToggle;
    bool _state;
    uint8_t _blinkCount;
    uint8_t _blinkTimes;
};  

#ifdef LED_PIN
extern LED led;
#endif

#endif