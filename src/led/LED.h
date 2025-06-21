#ifndef LED_H
#define LED_H

#include <Arduino.h>
#include "LEDTypes.h"  // 包含LED类型定义

class LED
{
public:
    LED(uint8_t pin);
    void begin();
    void loop();
    void setMode(LEDMode mode);
    void initBlink(uint8_t times);

private:
    static const unsigned long BLINK_INTERVAL = 100;    // 闪烁间隔(ms)
    static const unsigned long PATTERN_INTERVAL = 1000; // 整体模式重复间隔
    static const unsigned long INIT_BLINK_INTERVAL = 200; // 200ms闪烁间隔

    uint8_t _pin;
    LEDMode _mode;
    unsigned long _lastToggle;
    bool _state;
    uint8_t _blinkCount;
    uint8_t _blinkTimes;
};  

#ifdef LED_PIN
extern LED led;
#endif

#endif