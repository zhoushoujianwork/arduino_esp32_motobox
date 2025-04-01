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
    Serial.println("[PWMLED] 构造函数初始化完成");
}

void PWMLED::begin() {
    // 配置PWM通道
    ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
    
    // 将PWM通道附加到GPIO引脚
    ledcAttachPin(_pin, PWM_CHANNEL);
    
    // 初始状态设置为关闭
    setBrightness(0);
    Serial.printf("[PWMLED] 初始化完成，引脚: %d, PWM通道: %d\n", _pin, PWM_CHANNEL);
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
        
        // 根据模式设置初始亮度
        switch (mode) {
            case OFF:
                setBrightness(0);
                break;
            case RED:
                setBrightness(COLOR_RED);
                break;
            case GREEN:
                setBrightness(COLOR_GREEN);
                break;
            case BLUE:
                setBrightness(COLOR_BLUE);
                break;
            case YELLOW:
                setBrightness(COLOR_YELLOW);
                break;
            case BREATH:
            case RAINBOW:
                setBrightness(0);
                break;
        }
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

void PWMLED::setBrightness(uint8_t brightness) {
    ledcWrite(PWM_CHANNEL, brightness);
    // 每100次更新打印一次亮度值，避免打印太多
    static uint32_t printCount = 0;
    if (++printCount % 100 == 0) {
        Serial.printf("[PWMLED] 设置亮度: %d\n", brightness);
    }
}

void PWMLED::updateBreath() {
    if (_increasing) {
        _brightness += BREATH_STEP;
        if (_brightness >= 255) {
            _brightness = 255;
            _increasing = false;
        }
    } else {
        _brightness -= BREATH_STEP;
        if (_brightness <= 0) {
            _brightness = 0;
            _increasing = true;
        }
    }
    setBrightness(_brightness);
}

void PWMLED::updateRainbow() {
    // 使用正弦函数创建平滑的彩虹效果
    _hue = (_hue + RAINBOW_STEP) % 256;
    
    // 使用正弦函数创建更明显的颜色变化
    float phase = _hue * 2 * PI / 256;
    
    // 计算RGB值
    uint8_t r = (uint8_t)(127.5 + 127.5 * sin(phase));
    uint8_t g = (uint8_t)(127.5 + 127.5 * sin(phase + 2 * PI / 3));
    uint8_t b = (uint8_t)(127.5 + 127.5 * sin(phase + 4 * PI / 3));
    
    // 由于是单引脚LED，我们取RGB的最大值作为亮度
    _brightness = max(max(r, g), b);
    
    // 每100次更新打印一次详细信息
    static uint32_t printCount = 0;
    if (++printCount % 100 == 0) {
        Serial.printf("[PWMLED] 彩虹效果 - 色相: %d, RGB: (%d,%d,%d), 亮度: %d\n", 
                     _hue, r, g, b, _brightness);
    }
    
    // 应用亮度
    setBrightness(_brightness);
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