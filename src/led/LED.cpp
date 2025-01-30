#include "LED.h"

LED::LED(uint8_t pin) : _pin(pin), _mode(OFF), _lastToggle(0), _state(false), _blinkCount(0)
{
}

void LED::begin()
{
    pinMode(_pin, OUTPUT);
    digitalWrite(_pin, LOW);
}

void LED::setMode(Mode mode)
{
    if (_mode != mode)
    {
        _mode = mode;
        _state = false;
        _blinkCount = 0;
        digitalWrite(_pin, LOW);
    }
}

void LED::loop()
{
    unsigned long currentMillis = millis();

    switch (_mode)
    {
    case ON:
        digitalWrite(_pin, LOW);
        break;

    case OFF:
        digitalWrite(_pin, HIGH);
        break;

    case BLINK:
        if (currentMillis - _lastToggle >= PATTERN_INTERVAL)
        {
            _state = true;
            digitalWrite(_pin, HIGH);
            _lastToggle = currentMillis;
        }
        else if (_state && currentMillis - _lastToggle >= BLINK_INTERVAL)
        {
            _state = false;
            digitalWrite(_pin, LOW);
        }
        break;

    case BLINK_DUAL:
        if (currentMillis - _lastToggle >= PATTERN_INTERVAL)
        {
            _blinkCount = 0;
            _state = true;
            digitalWrite(_pin, LOW);
            _lastToggle = currentMillis;
        }
        else if (currentMillis - _lastToggle < 800)
        {
            unsigned long phase = (currentMillis - _lastToggle) / 200;
            if (phase != _blinkCount)
            {
                _blinkCount = phase;
                digitalWrite(_pin, phase % 2 == 0 ? LOW : HIGH);
            }
        }
        else if (_state)
        {
            _state = false;
            digitalWrite(_pin, HIGH);
        }
        break;
    }
}