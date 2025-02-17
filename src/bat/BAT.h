#ifndef BAT_H
#define BAT_H

#include <Arduino.h>
#include "device.h"

#define FILTER_WINDOW_SIZE 5 // 与cpp文件保持一致

class BAT
{
public:
    BAT(int pin, int min_voltage, int max_voltage);
    void loop();
    void print_voltage();

private:
    int pin;
    int min_voltage;
    int max_voltage;
    int voltage;

    // 新增滤波缓冲区相关变量
    int voltage_buffer[FILTER_WINDOW_SIZE];
    int buffer_index;
};

#endif
