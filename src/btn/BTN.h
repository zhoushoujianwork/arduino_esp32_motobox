#ifndef BTN_H
#define BTN_H

class BTN {
    public:
        BTN(int pin);
        bool isPressed();
        bool isReleased();
        bool isClicked();
        bool isLongPressed();
       
    private:
        int pin;
};

#endif