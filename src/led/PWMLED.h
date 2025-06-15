#ifndef PWM_LED_H
#define PWM_LED_H

#include <Arduino.h>
#include <FastLED.h>

// 定义一个无效的引脚常量，用于动态设置引脚
#define PIN_UNDEFINED 255

class PWMLED {
public:
    // LED显示模式
    enum Mode {
        OFF,           // 关闭
        SOLID,         // 常亮（稳定状态）
        BREATH,        // 呼吸（过渡状态）
        BLINK_SLOW,    // 慢闪（连接中）
        BLINK_FAST,    // 快闪（错误）
    };

    // 模块颜色定义
    enum ModuleColor {
        NONE,          // 关闭
        WHITE,         // GPS模块 - 白色
        BLUE,          // WiFi模块 - 蓝色
        YELLOW,        // 4G模块 - 黄色
        PURPLE,        // IMU模块 - 紫色
        GREEN,         // 系统正常状态 - 绿色
        RED           // 系统错误状态 - 红色
    };

    // RGB颜色结构
    struct RGB {
        uint8_t r, g, b;
    };

    // 常量定义
    static const uint8_t MAX_BRIGHTNESS = 255;
    static const uint8_t DEFAULT_BRIGHTNESS = 25;
    static const uint8_t NUM_LEDS = 1;
    
    // 效果参数
    static const uint8_t BREATH_STEP = 5;
    static const uint8_t BREATH_INTERVAL = 40;
    static const uint16_t BLINK_SLOW_INTERVAL = 1000;
    static const uint16_t BLINK_FAST_INTERVAL = 200;

    // 构造函数
    PWMLED(uint8_t pin);
    
    // 公共方法
    void begin();
    void loop();
    void setMode(Mode mode);
    void setColor(ModuleColor color);
    void setBrightness(uint8_t brightness);

private:
    // 成员变量
    uint8_t _pin;
    Mode _mode;
    ModuleColor _currentColor;
    uint8_t _brightness;
    unsigned long _lastUpdate;
    bool _blinkState;
    uint8_t _breathValue;
    bool _breathIncreasing;
    CRGB _leds[NUM_LEDS];

    // 颜色映射表声明
    static const RGB COLOR_MAP[];  // 只声明，不定义

    // 私有方法
    void updateBreathEffect();
    void updateBlinkEffect();
    void updateColor();
    void showLED();
};

#ifdef PWM_LED_PIN
extern PWMLED pwmLed;
#endif

#endif 