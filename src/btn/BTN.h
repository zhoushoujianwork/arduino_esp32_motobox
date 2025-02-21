#ifndef BTN_H
#define BTN_H

class BTN
{
public:
    BTN(int pin);
    bool isPressed();
    bool isReleased();
    bool isClicked();
    bool isLongPressed();
    void loop();

private:
    int pin;
    bool currentState = 1;  // 初始化为 HIGH
    unsigned long pressStartTime = 0;
    static const unsigned long LONG_PRESS_TIME = 3000; // 3秒定义为长按
    static const unsigned long DEBOUNCE_DELAY = 50;    // 添加消抖延时常量
};

#endif