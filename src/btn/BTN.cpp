#include "BTN.h"
#include <Arduino.h>
#include "gps/GPS.h"
#include "wifi/WifiManager.h"

BTN::BTN(int pin)
{
    this->pin = pin;
    pinMode(pin, INPUT_PULLUP); // 设置为上拉输入模式
    currentState = digitalRead(pin); // 初始化 currentState
    Serial.println("[BTN] 初始化开始");
    Serial.printf("[BTN] 引脚: %d\n", pin);
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

BTN::ButtonState BTN::getState()
{
    static unsigned long lastClickTime = 0;
    static bool isFirstClick = true;
    ButtonState currentState = NONE;

    if (isClicked()) {
        if (isFirstClick) {
            isFirstClick = false;
            lastClickTime = millis();
        } else {
            if (millis() - lastClickTime < 500) { // 500ms内第二次点击
                currentState = DOUBLE_CLICK;
                isFirstClick = true;
            } else {
                currentState = SINGLE_CLICK;
                isFirstClick = false;
                lastClickTime = millis();
            }
        }
    } else if (isLongPressed()) {
        currentState = LONG_PRESS;
        isFirstClick = true;
    } else if (!isFirstClick && millis() - lastClickTime >= 500) {
        currentState = SINGLE_CLICK;
        isFirstClick = true;
    }

    if (currentState != NONE) {
        lastState = currentState;
    }
    return currentState;
}

// 新增：静态按钮事件处理函数
void BTN::handleButtonEvents() {
#ifdef BTN_PIN
#if defined(MODE_ALLINONE) || defined(MODE_SERVER)
    // 获取按钮状态
    extern BTN button;
    extern GPS gps;
    extern WiFiConfigManager wifiManager;
    BTN::ButtonState state = button.getState();

    // 处理按钮事件
    switch (state)
    {
    case BTN::SINGLE_CLICK:
    {
        Serial.println("[按钮] 单击 重启");
        int hz = gps.changeHz();
        Serial.printf("[GPS] 当前频率: %dHz\n", hz);
        esp_restart();
        break;
    }

    case BTN::DOUBLE_CLICK:
    {
        Serial.println("[按钮] 双击");
        int baudRate = gps.changeBaudRate();
        Serial.printf("[GPS] 当前波特率: %d\n", baudRate);
        break;
    }

    case BTN::LONG_PRESS:
        Serial.println("[按钮] 长按");
        wifiManager.reset();
        break;

    default:
        break;
    }
#endif
#endif
}
