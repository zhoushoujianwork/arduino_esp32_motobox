#ifndef PWM_LED_H
#define PWM_LED_H

#include <Arduino.h>

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
    
    // PWM 相关参数
    static const uint8_t PWM_CHANNEL = 0;
    static const uint8_t PWM_RESOLUTION = 8;  // 8位分辨率 (0-255)
    static const uint32_t PWM_FREQ = 5000;    // 5KHz频率
    
    // 呼吸灯参数
    static const uint8_t BREATH_STEP = 5;     // 每次亮度变化步长
    static const uint8_t BREATH_INTERVAL = 50; // 亮度更新间隔(ms)
    
    // 彩虹效果参数
    static const uint8_t RAINBOW_STEP = 1;    // 色相变化步长
    static const uint8_t RAINBOW_INTERVAL = 20; // 彩虹效果更新间隔(ms)
    
    // 颜色参数 (10% 亮度)
    static const uint8_t COLOR_RED = 25;      // 255 * 10% ≈ 25
    static const uint8_t COLOR_GREEN = 25;    // 255 * 10% ≈ 25
    static const uint8_t COLOR_BLUE = 25;     // 255 * 10% ≈ 25
    static const uint8_t COLOR_YELLOW = 25;   // 黄色使用相同亮度
    
    // 辅助函数
    void setBrightness(uint8_t brightness);
    void updateBreath();
    void updateRainbow();
    const char* modeToString(Mode mode);
};

#endif 