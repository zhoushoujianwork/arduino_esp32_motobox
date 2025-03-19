#include "LED.h"

LED::LED(uint8_t pin) : _pin(pin), _mode(OFF), _lastToggle(0), _state(false), _blinkCount(0)
{
    _lastMode = OFF; // 初始化上一次模式为OFF
}

void LED::begin()
{
    // 配置LED引脚为输出
    pinMode(_pin, OUTPUT);
    digitalWrite(_pin, HIGH); // 初始状态为关闭 (LED是低电平点亮的)
}

void LED::setMode(Mode mode)
{
    if (_mode != mode)
    {
        // 记录模式变化并输出
        Mode oldMode = _mode;
        _mode = mode;
        _state = false;
        _blinkCount = 0;
        
        // 输出模式变化信息
        Serial.print("LED模式变化: ");
        Serial.print(modeToString(oldMode));
        Serial.print(" -> ");
        Serial.println(modeToString(mode));
        
        // 根据模式设置初始状态
        switch (mode) {
            case OFF:
                digitalWrite(_pin, HIGH); // LED关闭 (高电平)
                break;
            case ON:
                digitalWrite(_pin, LOW); // LED打开 (低电平)
                break;
            default:
                digitalWrite(_pin, HIGH); // 默认关闭
                break;
        }
    }
}

void LED::loop()
{
    unsigned long currentMillis = millis();

    switch (_mode)
    {
    case ON:
        digitalWrite(_pin, LOW); // LED打开 (低电平)
        break;

    case OFF:
        digitalWrite(_pin, HIGH); // LED关闭 (高电平)
        break;

    case BLINK_SINGLE:
        if (currentMillis - _lastToggle >= PATTERN_INTERVAL)
        {
            _state = true;
            digitalWrite(_pin, HIGH); // LED关闭 (高电平)
            _lastToggle = currentMillis;
        }
        else if (_state && currentMillis - _lastToggle >= BLINK_INTERVAL)
        {
            _state = false;
            digitalWrite(_pin, LOW); // LED打开 (低电平)
        }
        break;

    case BLINK_DUAL:
        if (currentMillis - _lastToggle >= PATTERN_INTERVAL)
        {
            _blinkCount = 0;
            _state = true;
            digitalWrite(_pin, LOW); // LED打开 (低电平)
            _lastToggle = currentMillis;
        }
        else if (currentMillis - _lastToggle < 800)
        {
            unsigned long phase = (currentMillis - _lastToggle) / 200;
            if (phase != _blinkCount)
            {
                _blinkCount = phase;
                // 偶数阶段亮，奇数阶段灭
                digitalWrite(_pin, phase % 2 == 0 ? LOW : HIGH);
            }
        }
        else if (_state)
        {
            _state = false;
            digitalWrite(_pin, HIGH); // LED关闭 (高电平)
        }
        break;
    }
}

// 添加一个辅助函数将模式转换为字符串
const char* LED::modeToString(Mode mode) {
    switch (mode) {
        case OFF: return "关闭";
        case ON: return "常亮";
        case BLINK_SINGLE: return "单闪";
        case BLINK_DUAL: return "双闪";
        default: return "未知";
    }
}