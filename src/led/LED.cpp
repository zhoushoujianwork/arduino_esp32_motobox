#include "LED.h"

#ifdef LED_PIN
LED led(LED_PIN);
#endif

LED::LED(uint8_t pin) : _pin(pin), _mode(LED_OFF), _lastToggle(0), _state(false), _blinkCount(0), _blinkTimes(0)
{
}

void LED::begin()
{
    pinMode(_pin, OUTPUT);
    digitalWrite(_pin, HIGH); // 初始状态设为HIGH（LED灭）
    Serial.printf("[LED] 引脚: %d\n", _pin);
}

void LED::setMode(LEDMode mode)
{
    if (_mode != mode)
    {
        _mode = mode;
        _lastToggle = 0; // 切换模式时重置计时器
        _blinkCount = 0;
        _state = false;
        digitalWrite(_pin, HIGH); // 切换模式时设为HIGH（LED灭）
    }
}

void LED::loop()
{
    unsigned long currentMillis = millis();

    switch (_mode)
    {
    case LED_ON:
        _state = true;
        break;

    case LED_OFF:
        _state = false;
        break;

    case LED_BLINK_SINGLE:
        if (currentMillis - _lastToggle >= PATTERN_INTERVAL)
        {
            _state = true;
            _lastToggle = currentMillis;
        }
        else if (_state && currentMillis - _lastToggle >= BLINK_INTERVAL)
        {
            _state = false;
        }
        break;

    case LED_BLINK_DUAL:
        if (currentMillis - _lastToggle >= PATTERN_INTERVAL)
        {
            _blinkCount = 0;
            _state = true;
            _lastToggle = currentMillis;
        }
        else if (currentMillis - _lastToggle < 600) 
        {
            unsigned long phase = (currentMillis - _lastToggle) / 150; 
            if (phase != _blinkCount)
            {
                _blinkCount = phase;
                _state = (_blinkCount % 2 == 0);
            }
        }
        else
        {
            _state = false;
        }
        break;

    case LED_BLINK_FAST:
        if (currentMillis - _lastToggle >= 100)
        {
            _state = !_state;
            _lastToggle = currentMillis;
        }
        break;

    case LED_BLINK_5_SECONDS:
        if (currentMillis - _lastToggle >= 5000)
        {
            _state = !_state;
            _lastToggle = currentMillis;
        }
        break;

    case LED_BLINK_SLOW:
        if (currentMillis - _lastToggle > 500)
        {
            _state = !_state;
            _lastToggle = currentMillis;
        }
        break;

    default:
        _state = false;
        break;
    }

    // 修改LED控制逻辑：低电平触发（共阳极LED）
    digitalWrite(_pin, _state ? LOW : HIGH);
}

// 初始化闪烁,最原始的闪烁方式，配合 delay
void LED::initBlink(uint8_t times) {
    for (int i = 0; i < times; i++) {
        digitalWrite(_pin, LOW);  // LED亮
        delay(INIT_BLINK_INTERVAL);
        digitalWrite(_pin, HIGH); // LED灭
        delay(INIT_BLINK_INTERVAL);
    }
    Serial.println("初始化闪烁完成");
}

const char* modeToString(LEDMode mode) {
    switch (mode) {
        case LED_OFF: return "关闭";
        case LED_ON: return "常亮";
        case LED_BLINK_SINGLE: return "单闪";
        case LED_BLINK_DUAL: return "双闪";
        case LED_BLINK_FAST: return "快闪";
        case LED_BLINK_5_SECONDS: return "5秒闪烁";
        case LED_BLINK_SLOW: return "慢闪";
        default: return "未知";
    }
}