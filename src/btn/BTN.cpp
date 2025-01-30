#include "BTN.h"
#include <Arduino.h>

// 定义一些常量
const unsigned long DEBOUNCE_DELAY = 50;    // 消抖延时(毫秒)
const unsigned long LONG_PRESS_TIME = 1000; // 长按时间阈值(毫秒)
const unsigned long MULTI_CLICK_TIME = 300; // 多击间隔时间(毫秒)

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