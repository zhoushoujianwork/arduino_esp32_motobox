#ifndef BAT_H
#define BAT_H

#include <Arduino.h>
#include "device.h"

#define FILTER_WINDOW_SIZE 20 // 增大滤波窗口大小
#define EMA_ALPHA 0.1f        // 指数平均滤波系数，越小滤波效果越强
#define OUTPUT_DIVIDER 5      // 输出分频器，每N次计算更新一次输出
#define MAX_VOLTAGE_JUMP 50   // 最大允许电压跳变值(mV)

class BAT
{
public:
    BAT(int pin);
    void loop();
    void print_voltage();

private:
    int pin;
    int min_voltage;
    int max_voltage;
    int voltage;        // 简单平均滤波后的电压

    // 新增滤波相关变量
    int voltage_buffer[FILTER_WINDOW_SIZE];
    int buffer_index;
    
    // 增强滤波相关变量
    int ema_voltage;         // 指数移动平均滤波后的电压值
    int last_output_voltage; // 最后一次输出的电压值
    int stable_voltage;      // 稳定输出的电压值
    int output_counter;      // 输出分频计数器
};

#ifdef BAT_PIN
extern BAT bat;
#endif

#endif
