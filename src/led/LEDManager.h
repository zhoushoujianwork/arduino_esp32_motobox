#ifndef LED_MANAGER_H
#define LED_MANAGER_H

#include <Arduino.h>
#include "LEDTypes.h"  // 包含LED类型定义

class LEDManager {
public:
    LEDManager();
    void begin();
    void setLEDState(LEDMode mode, LEDColor color = LED_COLOR_GREEN, uint8_t brightness = 10);
    void loop();
    
private:
    LEDMode _mode;
    LEDColor _color;
    uint8_t _brightness;

    void updateLED();
};

#if defined(LED_PIN) || defined(PWM_LED_PIN)
extern LEDManager ledManager;
#endif

#endif
