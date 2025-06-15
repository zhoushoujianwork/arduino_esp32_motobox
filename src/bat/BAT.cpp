#include "BAT.h"

BAT bat(BAT_PIN);

// 3300, 4200
BAT::BAT(int pin)
    : pin(pin),
#ifdef BAT_MIN_VOLTAGE
      min_voltage(BAT_MIN_VOLTAGE),
#else
      min_voltage(2900),
#endif
#ifdef BAT_MAX_VOLTAGE
      max_voltage(BAT_MAX_VOLTAGE),
#else
      max_voltage(3233),
#endif
      buffer_index(0),
      voltage(0),
      ema_voltage(0),
      last_output_voltage(0),
      stable_voltage(0),
      output_counter(0)
{
    // 构造函数只保存参数，不做硬件/外设操作
}

void BAT::begin()
{
    pinMode(pin, INPUT);
    // 初始化缓冲区
    memset(voltage_buffer, 0, sizeof(voltage_buffer));
    Serial.println("[BAT] 初始化开始");
    Serial.printf("[BAT] 引脚: %d, 最小电压: %d, 最大电压: %d\n", pin, min_voltage, max_voltage);
    Serial.println("[BAT] 初始化完成");
}   

void BAT::loop()
{
    static constexpr float VOLTAGE_MULTIPLIER = 2.0f; // 电压倍数常量
    static constexpr int ADC_MAX_VALUE = 4095;        // ADC最大值

    // 读取原始ADC值并转换为电压
    int analogVolts = analogReadMilliVolts(this->pin);
    int current_voltage = analogVolts * VOLTAGE_MULTIPLIER;

    // 1. 滑动窗口平均滤波 - 使用运行总和优化计算
    static int running_sum = 0;

    // 初始化时填充缓冲区
    if (running_sum == 0 && voltage == 0)
    {
        for (int i = 0; i < FILTER_WINDOW_SIZE; i++)
        {
            voltage_buffer[i] = current_voltage;
        }
        running_sum = current_voltage * FILTER_WINDOW_SIZE;
        voltage = current_voltage;
        ema_voltage = current_voltage;
        last_output_voltage = current_voltage;
        stable_voltage = current_voltage;
    }
    else
    {
        running_sum -= voltage_buffer[buffer_index]; // 减去旧值
        running_sum += current_voltage;              // 加上新值
        voltage_buffer[buffer_index] = current_voltage;
        buffer_index = (buffer_index + 1) % FILTER_WINDOW_SIZE;

        // 计算滑动窗口平均值
        voltage = running_sum / FILTER_WINDOW_SIZE;
    }

    // 2. 指数移动平均滤波 (EMA)
    if (ema_voltage == 0)
    {
        ema_voltage = voltage; // 初始化
    }
    else
    {
        // 异常值检测 - 如果变化太大，减小其影响
        int voltage_diff = abs(voltage - ema_voltage);
        float alpha = EMA_ALPHA;

        // 如果变化超过阈值，降低该采样的权重
        if (voltage_diff > MAX_VOLTAGE_JUMP)
        {
            alpha = EMA_ALPHA * 0.5f; // 降低异常值的权重
        }

        // 应用EMA滤波
        ema_voltage = (int)(alpha * voltage + (1.0f - alpha) * ema_voltage);
    }

    // 3. 输出分频 - 每N次计算才更新一次输出值
    output_counter = (output_counter + 1) % OUTPUT_DIVIDER;
    if (output_counter == 0)
    {
        int output_diff = abs(ema_voltage - last_output_voltage);

        // 只有当变化足够明显时才更新输出
        if (output_diff > 5 || last_output_voltage == 0)
        {
            last_output_voltage = ema_voltage;
        }

        // 最终用于显示和状态更新的稳定电压值
        stable_voltage = last_output_voltage;
    }

    // 计算电池百分比（添加范围限制）
    int percentage = map(stable_voltage, min_voltage, max_voltage, 0, 100);
    percentage = constrain(percentage, 0, 100);

    // 更新设备状态
    device_state.battery_voltage = stable_voltage;
    device_state.battery_percentage = percentage;
}

void BAT::print_voltage()
{
    // 添加单位并优化输出格式
    Serial.printf("Battery: %dmV (%.1f%%)\n",
                  stable_voltage,
                  device_state.battery_percentage);
}