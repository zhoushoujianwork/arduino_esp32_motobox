#ifndef BAT_H
#define BAT_H

#include <Arduino.h>

class BAT
{
public:
    BAT(int pin, int min_voltage, int max_voltage);
    void loop();
    int getPercentage();
    void print_voltage();

private:
    int pin;
    int min_voltage;
    int max_voltage;
    int voltage;
};

#endif
