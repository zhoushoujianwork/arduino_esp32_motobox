#include "BTN.h"
#include <Arduino.h>

BTN::BTN(int pin)
{
    this->pin = pin;
    pinMode(pin, INPUT_PULLUP); // 设置为上拉输入模式
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
    static unsigned long lastPressTime = 0;
    static bool lastState = true;
    bool currentState = digitalRead(pin);

    if (lastState && !currentState)
    { // 按下瞬间
        lastPressTime = millis();
    }
    else if (!lastState && currentState)
    { // 释放瞬间
        if (millis() - lastPressTime < LONG_PRESS_TIME)
        {
            lastState = currentState;
            return true;
        }
    }
    lastState = currentState;
    return false;
}

bool BTN::isLongPressed()
{
    static unsigned long pressStartTime = 0;
    static bool longPressDetected = false;

    if (isPressed())
    {
        if (pressStartTime == 0)
        {
            pressStartTime = millis();
        }
        if (!longPressDetected && (millis() - pressStartTime >= LONG_PRESS_TIME))
        {
            longPressDetected = true;
            return true;
        }
    }
    else
    {
        pressStartTime = 0;
        longPressDetected = false;
    }
    return false;
}

void BTN::loop()
{
    static unsigned long lastDebounceTime = 0;
    static bool lastButtonState = HIGH;
    static unsigned long pressStartTime = 0;

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

            if (currentState == LOW)
            { // 按下
                pressStartTime = millis();
            }
        }
    }

    lastButtonState = reading;
}

bool BTN::isLongPress()
{
    if (currentState == LOW)
    {
        return (millis() - pressStartTime) >= LONG_PRESS_TIME;
    }
    return false;
}