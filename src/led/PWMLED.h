#ifndef PWM_LED_H
#define PWM_LED_H

#include <FastLED.h>
#include "LEDTypes.h"  // 包含LED类型定义

class PWMLED {
public:
    struct RGB {
        uint8_t r, g, b;
    };

    PWMLED(uint8_t pin);
    void begin();
    void loop();
    void setMode(LEDMode mode);
    void setColor(LEDColor color);
    void setBrightness(uint8_t brightness);
    void initRainbow();
private:
    static const uint8_t NUM_LEDS = 1;
    static const uint8_t DEFAULT_BRIGHTNESS = 10;
    static const uint8_t MAX_BRIGHTNESS = 255;
    static const uint8_t BREATH_STEP = 5;
    static const unsigned long BREATH_INTERVAL = 20;
    static const unsigned long BLINK_SLOW_INTERVAL = 1000;
    static const unsigned long BLINK_FAST_INTERVAL = 200;

    static const uint8_t RAINBOW_COLORS[7][3];

    uint8_t _pin;
    LEDMode _mode;
    LEDColor _currentColor;
    uint8_t _brightness;
    unsigned long _lastUpdate;
    bool _blinkState;
    uint8_t _breathValue;
    bool _breathIncreasing;
    uint8_t _rainbowIndex;
    CRGB _leds[NUM_LEDS];

    static const RGB COLOR_MAP[];
    void updateBreathEffect();
    void updateBlinkEffect();
    void updateColor();
    void showLED();
    void updateRainbow();
};

extern PWMLED pwmLed;

#endif 