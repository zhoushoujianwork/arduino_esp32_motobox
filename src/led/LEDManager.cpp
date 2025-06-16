#include "LEDManager.h"

#if defined(LED_PIN) || defined(PWM_LED_PIN)
LEDManager::LEDManager() : _mode(OFF), _color(GREEN), _brightness(10) {}
#endif


void LEDManager::begin() {
#ifdef PWM_LED_PIN
    pwmLed.begin();
#endif
#ifdef LED_PIN
    led.begin();
#endif
}

void LEDManager::initAnimation() {
#ifdef PWM_LED_PIN
    // PWM LED 彩虹色初始化
    pwmLed.initRainbow();
#endif

#ifdef LED_PIN
    // 普通 LED 闪烁两次
    led.initBlink(2);
#endif
}

void LEDManager::initComplete() {
#ifdef PWM_LED_PIN
    // PWM LED 设置为常亮，亮度5%
    pwmLed.setMode(PWMLED::SOLID);
    pwmLed.setColor(PWMLED::GREEN);
    pwmLed.setBrightness(13); // 5% of 255 ≈ 13
#endif

#ifdef LED_PIN
    // 普通 LED 设置为常亮
    led.setMode(LED::ON);
#endif
}

void LEDManager::setLEDState(Mode mode, Color color, uint8_t brightness) {
    _mode = mode;
    _color = color;
    _brightness = brightness;
    updateLED();
}

void LEDManager::loop() {
#ifdef PWM_LED_PIN
    // 获取当前设备状态
    const device_state_t& state = *get_device_state();
    
    // 检查是否所有模块都正常
    bool allModulesReady = state.gpsReady && 
                          state.wifiConnected && 
                          state.imuReady;

    if (allModulesReady) {
        setLEDState(BREATH, GREEN);
    } else if (state.wifiConnected) {
        setLEDState(ON, BLUE);
    } else if (state.gpsReady) {
        setLEDState(ON, WHITE);
    } else if (state.imuReady) {
        setLEDState(ON, PURPLE);
    }
    
    pwmLed.loop();
#endif

#ifdef LED_PIN
    led.loop();
#endif
}

PWMLED::Mode LEDManager::convertToPWMMode(Mode mode) {
    switch (mode) {
        case OFF: return PWMLED::OFF;
        case ON: return PWMLED::SOLID;
        case BLINK_SINGLE: return PWMLED::BLINK_SLOW;
        case BLINK_DUAL: return PWMLED::DUAL_BLINK;
        case BLINK_SLOW: return PWMLED::BLINK_SLOW;
        case BLINK_FAST: return PWMLED::BLINK_FAST;
        case BREATH: return PWMLED::BREATH;
        default: return PWMLED::OFF;
    }
}

LED::Mode LEDManager::convertToLEDMode(Mode mode) {
    switch (mode) {
        case OFF: return LED::OFF;
        case ON: return LED::ON;
        case BLINK_SINGLE: return LED::BLINK_SINGLE;
        case BLINK_DUAL: return LED::BLINK_DUAL;
        case BLINK_SLOW: return LED::BLINK_SINGLE;
        case BLINK_FAST: return LED::BLINK_DUAL;
        case BLINK_5_SECONDS: return LED::BLINK_5_SECONDS;
        default: return LED::OFF;
    }
}

void LEDManager::updateLED() {
#ifdef PWM_LED_PIN
    pwmLed.setMode(convertToPWMMode(_mode));
    pwmLed.setColor(static_cast<PWMLED::ModuleColor>(_color));
    pwmLed.setBrightness(_brightness);
#endif

#ifdef LED_PIN
    led.setMode(convertToLEDMode(_mode));
#endif
}

#if defined(LED_PIN) || defined(PWM_LED_PIN)
LEDManager ledManager;
#endif
