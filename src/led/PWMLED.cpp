#include "PWMLED.h"

PWMLED::PWMLED(uint8_t pin)
    : _pin(pin)
    , _mode(OFF)
    , _lastMode(OFF)
    , _lastUpdate(0)
    , _brightness(0)
    , _increasing(true)
    , _hue(0)
{
}

void PWMLED::begin() {
    // 初始化 FastLED
    FastLED.addLeds<WS2812, 45, GRB>(_leds, NUM_LEDS);
    Serial.printf("[PWMLED] FastLED 初始化完成，引脚: %d\n", _pin);
    
    // 测试LED
    _leds[0] = CRGB::White;
    FastLED.show();
    delay(500);
    _leds[0] = CRGB::Black;
    FastLED.show();
    
    Serial.println("[PWMLED] 初始化测试完成");
}

void PWMLED::setMode(Mode mode) {
    if (_mode != mode) {
        _lastMode = _mode;
        _mode = mode;
        _brightness = 0;
        _increasing = true;
        _hue = 0;
        
        Serial.printf("[PWMLED] 模式切换: %s -> %s\n", 
                     modeToString(_lastMode), 
                     modeToString(mode));
        
        // 根据模式设置初始颜色
        switch (mode) {
            case OFF:
                _leds[0] = CRGB::Black;
                break;
            case RED:
                _leds[0].setHSV(0, 255, MAX_BRIGHTNESS);
                break;
            case GREEN:
                _leds[0].setHSV(96, 255, MAX_BRIGHTNESS);
                break;
            case BLUE:
                _leds[0].setHSV(160, 255, MAX_BRIGHTNESS);
                break;
            case YELLOW:
                _leds[0].setHSV(64, 255, MAX_BRIGHTNESS);
                break;
            case BREATH:
            case RAINBOW:
                _leds[0] = CRGB::Black;
                break;
        }
        FastLED.show();
    }
}

void PWMLED::loop() {
    unsigned long currentMillis = millis();
    
    // 每1000ms打印一次loop执行状态
    static unsigned long lastDebugTime = 0;
    if (currentMillis - lastDebugTime >= 1000) {
        Serial.printf("[PWMLED] Loop执行中 - 当前模式: %s\n", modeToString(_mode));
        lastDebugTime = currentMillis;
    }
    
    // 处理呼吸灯效果
    if (_mode == BREATH) {
        if (currentMillis - _lastUpdate >= BREATH_INTERVAL) {
            updateBreath();
            _lastUpdate = currentMillis;
        }
    }
    // 处理彩虹效果
    else if (_mode == RAINBOW) {
        if (currentMillis - _lastUpdate >= RAINBOW_INTERVAL) {
            updateRainbow();
            _lastUpdate = currentMillis;
        }
    }
}

void PWMLED::updateBreath() {
    if (_increasing) {
        _brightness += BREATH_STEP;
        if (_brightness >= MAX_BRIGHTNESS) {
            _brightness = MAX_BRIGHTNESS;
            _increasing = false;
        }
    } else {
        _brightness -= BREATH_STEP;
        if (_brightness <= 0) {
            _brightness = 0;
            _increasing = true;
        }
    }
    
    // 使用 HSV 设置白色并控制亮度
    _leds[0].setHSV(0, 0, _brightness);  // 色相和饱和度为0表示白色
    FastLED.show();
    
    // 每100次更新打印一次亮度值
    static uint32_t printCount = 0;
    if (++printCount % 100 == 0) {
        Serial.printf("[PWMLED] 呼吸灯亮度: %d\n", _brightness);
    }
}

void PWMLED::updateRainbow() {
    // 使用 FastLED 的 HSV 转换
    _hue = (_hue + RAINBOW_STEP) % 256;
    _leds[0].setHSV(_hue, 255, MAX_BRIGHTNESS);
    FastLED.show();
    
    // 每100次更新打印一次详细信息
    static uint32_t printCount = 0;
    if (++printCount % 100 == 0) {
        Serial.printf("[PWMLED] 彩虹效果 - 色相: %d\n", _hue);
    }
}

const char* PWMLED::modeToString(Mode mode) {
    switch (mode) {
        case OFF: return "OFF";
        case RED: return "RED";
        case GREEN: return "GREEN";
        case BLUE: return "BLUE";
        case YELLOW: return "YELLOW";
        case BREATH: return "BREATH";
        case RAINBOW: return "RAINBOW";
        default: return "UNKNOWN";
    }
} 