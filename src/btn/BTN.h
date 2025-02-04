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
    bool currentState;
    unsigned long pressStartTime;
    static const unsigned long LONG_PRESS_TIME = 3000; // 3秒定义为长按
    static const unsigned long DEBOUNCE_DELAY = 50;    // 添加消抖延时常量
};

#endif