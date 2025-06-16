#ifndef LED_MANAGER_H
#define LED_MANAGER_H

#include "PWMLED.h"
#include "LED.h"
#include "../device.h"

class LEDManager {
public:
    // 统一的LED显示模式
    enum Mode {
        OFF,           // 关闭
        ON,            // 常亮
        BLINK_SINGLE,  // 单闪
        BLINK_DUAL,    // 双闪
        BLINK_SLOW,    // 慢闪
        BLINK_FAST,    // 快闪
        BREATH,        // 呼吸
        BLINK_5_SECONDS // 5秒闪烁
    };

    // 统一的LED颜色（仅PWM LED使用）
    enum Color {
        NONE,
        WHITE,  // GPS
        BLUE,   // WiFi
        YELLOW, // 4G
        PURPLE, // IMU
        GREEN,  // System OK
        RED     // System Error
    };

    LEDManager();
    void begin();
    void setLEDState(Mode mode, Color color = GREEN, uint8_t brightness = 10);
    void loop();
    

private:
    Mode _mode;
    Color _color;
    uint8_t _brightness;

    PWMLED::Mode convertToPWMMode(Mode mode);
    LED::Mode convertToLEDMode(Mode mode);
    void updateLED();
};

#if defined(LED_PIN) || defined(PWM_LED_PIN)
extern LEDManager ledManager;
#endif

#endif
