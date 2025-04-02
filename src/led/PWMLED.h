#ifndef PWM_LED_H
#define PWM_LED_H

#include <Arduino.h>
#include <FastLED.h>

class PWMLED {
public:
    enum Mode {
        OFF,           // 关闭
        RED,           // 红色
        GREEN,         // 绿色
        BLUE,          // 蓝色
        YELLOW,        // 黄色
        BREATH,        // 呼吸效果
        RAINBOW        // 彩虹效果
    };

    static const uint8_t MAX_BRIGHTNESS = 128;  // 固定最大亮度为 50%

    PWMLED(uint8_t pin);
    void begin();
    void setMode(Mode mode);
    void loop();

private:
    uint8_t _pin;
    Mode _mode;
    Mode _lastMode;
    unsigned long _lastUpdate;
    uint8_t _brightness;
    bool _increasing;
    uint8_t _hue;      // 用于彩虹效果的色相值
    
    // WS2812 LED 参数
    static const uint8_t NUM_LEDS = 1;    // LED数量
    CRGB _leds[NUM_LEDS];                 // LED数组
    
    // 效果参数
    static const uint8_t BREATH_STEP = 5;     // 每次亮度变化步长
    static const uint8_t BREATH_INTERVAL = 40; // 亮度更新间隔(ms)
    static const uint8_t RAINBOW_STEP = 2;     // 色相变化步长
    static const uint8_t RAINBOW_INTERVAL = 20; // 彩虹效果更新间隔(ms)
    
    // 辅助函数
    void updateBreath();
    void updateRainbow();
    const char* modeToString(Mode mode);
};

#endif 