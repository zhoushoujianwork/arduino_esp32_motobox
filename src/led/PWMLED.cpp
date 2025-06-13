#include "PWMLED.h"

#ifdef PWM_LED_PIN
PWMLED pwmLed(PWM_LED_PIN);
#endif

PWMLED::PWMLED(uint8_t pin)
    : _pin(pin)
    , _mode(OFF)
    , _lastMode(OFF)
    , _lastUpdate(0)
    , _brightness(DEFAULT_BRIGHTNESS)
    , _increasing(true)
    , _hue(0)
{
    Serial.println("[PWMLED] 初始化开始");
    Serial.printf("[PWMLED] 引脚: %d\n", _pin);

    // 初始化 FastLED，直接使用传入的引脚
    FastLED.addLeds<WS2812, PWM_LED_PIN, GRB>(_leds, NUM_LEDS);
    Serial.printf("[PWMLED] FastLED 初始化完成，引脚: %d\n", _pin);
    
    // 测试LED
    _leds[0] = CRGB::White;
    FastLED.show();
    delay(500);
    _leds[0] = CRGB::Black;
    FastLED.show();
    
    Serial.println("[PWMLED] 初始化测试完成");
}


void PWMLED::setMode(Mode mode, uint8_t brightness) {
    if (_mode != mode || brightness > 0) {
        _lastMode = _mode;
        _mode = mode;
        
        // 设置亮度，如果传入0则使用默认亮度
        if (brightness > 0) {
            _brightness = min(brightness, MAX_BRIGHTNESS);
        } else {
            _brightness = DEFAULT_BRIGHTNESS;
        }
        
        _increasing = true;
        _hue = 0;
        
        // Serial.printf("[PWMLED] 模式切换: %s -> %s, 亮度: %d\n", 
        //              modeToString(_lastMode), 
        //              modeToString(mode),
        //              _brightness);
        
        // 根据模式设置初始颜色
        switch (mode) {
            case OFF:
                _leds[0] = CRGB::Black;
                break;
            case RED:
                _leds[0].setHSV(0, 255, _brightness);
                break;
            case GREEN:
                _leds[0].setHSV(96, 255, _brightness);
                break;
            case BLUE:
                _leds[0].setHSV(160, 255, _brightness);
                break;
            case YELLOW:
                _leds[0].setHSV(64, 255, _brightness);
                break;
            case BREATH:
                _brightness = 0;  // 呼吸模式从0开始
                _leds[0] = CRGB::Black;
                break;
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
        // Serial.printf("[PWMLED] Loop执行中 - 当前模式: %s\n", modeToString(_mode));
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
    _leds[0].setHSV(_hue, 255, _brightness);  // 使用当前亮度
    FastLED.show();
    
    // 每100次更新打印一次详细信息
    // static uint32_t printCount = 0;
    // if (++printCount % 100 == 0) {
    //     Serial.printf("[PWMLED] 彩虹效果 - 色相: %d\n", _hue);
    // }
}

/*
* 切换模式
* 彩虹模式：RAINBOW
* 呼吸模式：BREATH
* 红色模式：RED
* 绿色模式：GREEN
* 蓝色模式：BLUE
* 黄色模式：YELLOW
* 关闭模式：OFF
* 按个轮换
*/
void PWMLED::changeMode() {
    _mode = static_cast<Mode>((_mode + 1) % (RAINBOW + 1));
    setMode(_mode, DEFAULT_BRIGHTNESS);  // 使用默认亮度
    Serial.printf("[PWMLED] 模式切换: %s\n", modeToString(_mode));
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

void PWMLED::setBrightness(uint8_t brightness) {
    if (brightness > 0 && brightness <= MAX_BRIGHTNESS) {
        _brightness = brightness;
        
        // 如果当前模式不是呼吸模式，立即应用新亮度
        if (_mode != BREATH && _mode != OFF) {
            switch (_mode) {
                case RED:
                    _leds[0].setHSV(0, 255, _brightness);
                    break;
                case GREEN:
                    _leds[0].setHSV(96, 255, _brightness);
                    break;
                case BLUE:
                    _leds[0].setHSV(160, 255, _brightness);
                    break;
                case YELLOW:
                    _leds[0].setHSV(64, 255, _brightness);
                    break;
                case RAINBOW:
                    _leds[0].setHSV(_hue, 255, _brightness);
                    break;
                default:
                    break;
            }
            FastLED.show();
        }
        
        Serial.printf("[PWMLED] 亮度设置为: %d\n", _brightness);
    }
}
