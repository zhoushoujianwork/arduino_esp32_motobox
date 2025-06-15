#include "PWMLED.h"

#ifdef PWM_LED_PIN
PWMLED pwmLed(PWM_LED_PIN);
#endif

// 定义颜色映射表
const PWMLED::RGB PWMLED::COLOR_MAP[] = {
    {0, 0, 0},       // NONE
    {255, 255, 255}, // WHITE (GPS)
    {0, 0, 255},     // BLUE (WiFi)
    {255, 255, 0},   // YELLOW (4G)
    {255, 0, 255},   // PURPLE (IMU)
    {0, 255, 0},     // GREEN (System OK)
    {255, 0, 0}      // RED (System Error)
};

PWMLED::PWMLED(uint8_t pin) :
    _pin(pin),
    _mode(OFF),
    _currentColor(NONE),
    _brightness(DEFAULT_BRIGHTNESS),
    _lastUpdate(0),
    _blinkState(false),
    _breathValue(0),
    _breathIncreasing(true)
{
}

void PWMLED::begin() {
    FastLED.addLeds<WS2812, PWM_LED_PIN, GRB>(_leds, NUM_LEDS);
    Serial.printf("[PWMLED] 初始化完成，引脚: %d\n", _pin);
}

void PWMLED::loop() {
    unsigned long currentMillis = millis();
    
    switch (_mode) {
        case BREATH:
            if (currentMillis - _lastUpdate >= BREATH_INTERVAL) {
                updateBreathEffect();
                _lastUpdate = currentMillis;
            }
            break;
            
        case BLINK_SLOW:
            if (currentMillis - _lastUpdate >= BLINK_SLOW_INTERVAL) {
                updateBlinkEffect();
                _lastUpdate = currentMillis;
            }
            break;
            
        case BLINK_FAST:
            if (currentMillis - _lastUpdate >= BLINK_FAST_INTERVAL) {
                updateBlinkEffect();
                _lastUpdate = currentMillis;
            }
            break;
    }
}

void PWMLED::setMode(Mode mode) {
    if (_mode != mode) {
        _mode = mode;
        _lastUpdate = 0;
        _blinkState = false;
        _breathValue = 0;
        _breathIncreasing = true;
        updateColor();
    }
}

void PWMLED::setColor(ModuleColor color) {
    if (_currentColor != color) {
        _currentColor = color;
        updateColor();
    }
}

void PWMLED::setBrightness(uint8_t brightness) {
    _brightness = min(brightness, MAX_BRIGHTNESS);
    updateColor();
}

void PWMLED::updateBreathEffect() {
    if (_breathIncreasing) {
        _breathValue += BREATH_STEP;
        if (_breathValue >= _brightness) {
            _breathValue = _brightness;
            _breathIncreasing = false;
        }
    } else {
        if (_breathValue <= BREATH_STEP) {
            _breathValue = 0;
            _breathIncreasing = true;
        } else {
            _breathValue -= BREATH_STEP;
        }
    }
    showLED();
}

void PWMLED::updateBlinkEffect() {
    _blinkState = !_blinkState;
    showLED();
}

void PWMLED::updateColor() {
    const RGB& rgb = COLOR_MAP[_currentColor];
    _leds[0].setRGB(rgb.r, rgb.g, rgb.b);
    showLED();
}

void PWMLED::showLED() {
    uint8_t actualBrightness = _brightness;
    
    switch (_mode) {
        case OFF:
            actualBrightness = 0;
            break;
        case BREATH:
            actualBrightness = _breathValue;
            break;
        case BLINK_SLOW:
        case BLINK_FAST:
            actualBrightness = _blinkState ? _brightness : 0;
            break;
        case SOLID:
            // 使用设置的亮度
            break;
    }
    
    _leds[0].nscale8(actualBrightness);
    FastLED.show();
}
