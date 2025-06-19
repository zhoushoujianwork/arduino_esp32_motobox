#ifndef BAT_H
#define BAT_H

#include <Arduino.h>
#include "device.h"
#include "utils/PreferencesUtils.h"

#define FILTER_WINDOW_SIZE 20 // 增大滤波窗口大小
#define EMA_ALPHA 0.1f        // 指数平均滤波系数，越小滤波效果越强
#define OUTPUT_DIVIDER 5      // 输出分频器，每N次计算更新一次输出
#define MAX_VOLTAGE_JUMP 50   // 最大允许电压跳变值(mV)

class BAT
{
public:
    BAT(int pin);
    void begin();
    void loop();
    void print_voltage();
    
    // 添加调试控制
    void setDebug(bool debug) { _debug = debug; }

private:
    const int pin;
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

    // 调试相关
    bool _debug;
    unsigned long _lastDebugPrintTime;
    void debugPrint(const String& message);
    
    // 自动校准相关
    static constexpr int VOLTAGE_MAX_LIMIT = 4500;  // 电压上限保护
    static constexpr int VOLTAGE_MIN_LIMIT = 2500;  // 电压下限保护
    static constexpr int CALIBRATION_INTERVAL = 60000;  // 校准间隔(ms)
    static constexpr int STABLE_COUNT = 10;  // 稳定计数阈值
    static constexpr unsigned long DEBUG_PRINT_INTERVAL = 1000; // 调试打印间隔(ms)
    
    int observed_max;  // 观察到的最大电压
    int observed_min;  // 观察到的最小电压
    unsigned long last_calibration;  // 上次校准时间
    int stable_count;  // 稳定计数器
    
    void updateCalibration();  // 更新校准
    void loadCalibration();    // 加载校准数据
    void saveCalibration();    // 保存校准数据
};

extern BAT bat;

#endif
