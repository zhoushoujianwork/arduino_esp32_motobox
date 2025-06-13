#ifndef BTN_H
#define BTN_H

class BTN
{
public:
    enum ButtonState {
        SINGLE_CLICK,
        DOUBLE_CLICK,
        LONG_PRESS,
        NONE
    };

    BTN(int pin);
    ButtonState getState();
    bool isPressed();
    bool isReleased();
    bool isClicked();
    bool isLongPressed();
    void loop();

    // 新增：静态按钮事件处理函数
    static void handleButtonEvents();

private:
    int pin;
    bool currentState = 1;  // 初始化为 HIGH
    unsigned long pressStartTime = 0;
    static const unsigned long LONG_PRESS_TIME = 3000; // 3秒定义为长按
    static const unsigned long DEBOUNCE_DELAY = 50;    // 添加消抖延时常量
    ButtonState lastState = NONE;
};

#ifdef BTN_PIN
extern BTN button;
#endif

#endif