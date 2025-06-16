#include "LED.h"

#ifdef LED_PIN
LED led(LED_PIN);
#endif

LED::LED(uint8_t pin) : _pin(pin), _mode(OFF), _lastToggle(0), _state(false), _blinkCount(0)
{
    _lastMode = OFF; // 初始化上一次模式为OFF
}

void LED::begin()
{
    pinMode(_pin, OUTPUT);
    digitalWrite(_pin, HIGH); // 初始状态为关闭 (LED是低电平点亮的)
    Serial.printf("[LED] 引脚: %d\n", _pin);
}

void LED::setMode(Mode mode)
{
    if (_mode != mode)
    {
        _mode = mode;
        _state = false;
        _blinkCount = 0;
        
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
        // 修改双闪模式：两次短闪后长间隔
        if (currentMillis - _lastToggle >= PATTERN_INTERVAL)
        {
            _blinkCount = 0;
            _state = true;
            digitalWrite(_pin, LOW); // LED打开 (低电平)
            _lastToggle = currentMillis;
        }
        else if (currentMillis - _lastToggle < 600) // 缩短双闪的总时间
        {
            unsigned long phase = (currentMillis - _lastToggle) / 150; // 加快闪烁速度
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

    case BLINK_FAST:
        // 修改快闪模式：连续快速闪烁
        if (currentMillis - _lastToggle >= 100) // 100ms 的快速闪烁
        {
            _state = !_state;
            digitalWrite(_pin, _state ? LOW : HIGH);
            _lastToggle = currentMillis;
        }
        break;

    case BLINK_5_SECONDS:
        if (currentMillis - _lastToggle >= 5000)
        {
            _state = !_state;
            digitalWrite(_pin, _state ? LOW : HIGH);
            _lastToggle = currentMillis;
        }
        break;
    }
}

// 初始化闪烁,最原始的闪烁方式，配合 delay
void LED::initBlink(uint8_t times) {
    for (int i = 0; i < times; i++) {
        digitalWrite(_pin, LOW); // LED打开 (低电平)
        delay(100);
        digitalWrite(_pin, HIGH); // LED关闭 (高电平)
        delay(100);
    }
    delay(100);
    Serial.println("初始化闪烁完成");
}

// 添加一个辅助函数将模式转换为字符串
const char* LED::modeToString(Mode mode) {
    switch (mode) {
        case OFF: return "关闭";
        case ON: return "常亮";
        case BLINK_SINGLE: return "单闪";
        case BLINK_DUAL: return "双闪";
        case BLINK_FAST: return "快闪";
        case BLINK_5_SECONDS: return "5秒闪烁";
        default: return "未知";
    }
}