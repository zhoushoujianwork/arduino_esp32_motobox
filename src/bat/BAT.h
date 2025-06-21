#ifndef BAT_H
#define BAT_H

#include <Arduino.h>
#include "device.h"
#include "utils/PreferencesUtils.h"

#define FILTER_WINDOW_SIZE 20 // 滤波窗口大小
#define EMA_ALPHA 0.1f       // 指数平均滤波系数
#define OUTPUT_DIVIDER 5     // 输出分频器
#define MAX_VOLTAGE_JUMP 50  // 最大允许电压跳变值(mV)

class BAT
{
public:
    BAT(int adc_pin, int charging_pin);
    void begin();
    void loop();
    void print_voltage();
    
    // 添加调试控制
    void setDebug(bool debug) { _debug = debug; }

    // 新增：获取充电状态
    bool isCharging();

private:
    // 改为普通的静态常量数组声明
    static const int VOLTAGE_LEVELS[];
    static const int PERCENT_LEVELS[];
    static const int LEVEL_COUNT;
    
    const int pin;
    const int charging_pin; // 新增：充电状态引脚
    bool _is_charging;      // 新增：充电状态缓存

    int min_voltage;
    int max_voltage;
    int voltage;        // 简单平均滤波后的电压

    // 滤波相关
    int voltage_buffer[FILTER_WINDOW_SIZE];
    int buffer_index;
    int ema_voltage;         // 指数移动平均
    int last_output_voltage; // 最后输出电压
    int stable_voltage;      // 稳定电压
    int output_counter;      // 输出分频计数器

    // 调试相关
    bool _debug;
    void debugPrint(const String& message);
    
    // 校准相关
    static constexpr int VOLTAGE_MAX_LIMIT = 4500;
    static constexpr int VOLTAGE_MIN_LIMIT = 2500;
    static constexpr int CALIBRATION_INTERVAL = 60000;
    static constexpr int STABLE_COUNT = 10;
    
    int observed_max;
    int observed_min;
    unsigned long last_calibration;
    int stable_count;
    
    void updateCalibration();
    void loadCalibration();
    void saveCalibration();
    int calculatePercentage(int voltage);
};

extern BAT bat;

#endif
