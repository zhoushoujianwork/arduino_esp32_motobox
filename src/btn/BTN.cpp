#include "BTN.h"
#include <Arduino.h>

BTN::BTN(int pin)
{
    this->pin = pin;
    pinMode(pin, INPUT_PULLUP); // 设置为上拉输入模式
    currentState = digitalRead(pin); // 初始化 currentState
}

bool BTN::isPressed()
{
    return !digitalRead(pin); // 由于使用上拉电阻，按下时为LOW(0)
}

bool BTN::isReleased()
{
    return digitalRead(pin); // 释放时为HIGH(1)
}

bool BTN::isClicked()
{
    static bool lastState = HIGH;
    bool reading = digitalRead(pin);

    // 检测从高到低的瞬间变化（按下）
    if (lastState == HIGH && reading == LOW)
    {
        lastState = reading;
        return true;
    }

    lastState = reading;
    return false;
}

bool BTN::isLongPressed()
{
    if (isPressed())
    {
        if (pressStartTime == 0)
        {
            pressStartTime = millis();
        }

        if (millis() - pressStartTime >= LONG_PRESS_TIME)
        {
            return true;
        }
    }
    else
    {
        pressStartTime = 0;
    }
    return false;
}

void BTN::loop()
{
    static unsigned long lastDebounceTime = 0;
    static bool lastButtonState = HIGH;

    bool reading = digitalRead(pin);

    if (reading != lastButtonState)
    {
        lastDebounceTime = millis();
    }

    if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY)
    {
        if (reading != currentState)
        {
            currentState = reading;
        }
    }

    lastButtonState = reading;
}
