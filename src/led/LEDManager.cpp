#include "LEDManager.h"
#include "device.h"
#include "led/LED.h"
#include "led/PWMLED.h"

#if defined(LED_PIN) || defined(PWM_LED_PIN)
LEDManager::LEDManager() : _mode(LED_OFF), _color(LED_COLOR_GREEN), _brightness(10) {}
#endif


void LEDManager::begin() {
#ifdef PWM_LED_PIN
    pwmLed.begin();
     // PWM LED 彩虹色初始化
    pwmLed.initRainbow();
     // PWM LED 设置为常亮，亮度5%
    pwmLed.setMode(LED_ON);
    pwmLed.setColor(LED_COLOR_GREEN);
    pwmLed.setBrightness(13); // 5% of 255 ≈ 13

#endif
#ifdef LED_PIN
    led.begin();
     // 普通 LED 闪烁两次
    led.initBlink(2);
    // 普通 LED 设置为常亮
    led.setMode(LED_ON);
#endif
}


void LEDManager::setLEDState(LEDMode mode, LEDColor color, uint8_t brightness) {
    _mode = mode;
    _color = color;
    _brightness = brightness;
    device_state.led_mode = mode;
    updateLED();
}

void LEDManager::loop() {
#ifdef PWM_LED_PIN
    pwmLed.loop();
#endif

#ifdef LED_PIN
    led.loop();
#endif
}

void LEDManager::updateLED() {
#ifdef PWM_LED_PIN
    pwmLed.setMode(_mode);
    pwmLed.setColor(_color);
    pwmLed.setBrightness(_brightness);
#endif

#ifdef LED_PIN
    led.setMode(_mode);
#endif
}

#if defined(LED_PIN) || defined(PWM_LED_PIN)
LEDManager ledManager;
#endif
