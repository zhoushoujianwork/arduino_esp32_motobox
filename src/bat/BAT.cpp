#include "BAT.h"

// 3300, 4200
BAT::BAT(int pin, int min_voltage, int max_voltage)
    : pin(pin),
      min_voltage(min_voltage),
      max_voltage(max_voltage),
      buffer_index(0),
      voltage(0)
{
    // 使用memset初始化缓冲区更高效
    memset(voltage_buffer, 0, sizeof(voltage_buffer));
}

void BAT::loop()
{
    static constexpr float VOLTAGE_MULTIPLIER = 2.0f; // 电压倍数常量
    static constexpr int ADC_MAX_VALUE = 4095;        // ADC最大值

    int analogVolts = analogReadMilliVolts(this->pin);
    int current_voltage = analogVolts * VOLTAGE_MULTIPLIER;

    // 使用运行总和优化滤波计算
    static int running_sum = 0;
    running_sum -= voltage_buffer[buffer_index]; // 减去旧值
    running_sum += current_voltage;              // 加上新值

    voltage_buffer[buffer_index] = current_voltage;
    buffer_index = (buffer_index + 1) % FILTER_WINDOW_SIZE;

    this->voltage = running_sum / FILTER_WINDOW_SIZE; // 直接使用预计算总和

    // 计算电池百分比（添加范围限制）
    int percentage = map(this->voltage, min_voltage, max_voltage, 0, 100);
    percentage = constrain(percentage, 0, 100);

    device.get_device_state()->battery_voltage = this->voltage;
    device.get_device_state()->battery_percentage = percentage; // 修正百分比计算
}

void BAT::print_voltage()
{
    // 添加单位并优化输出格式
    Serial.printf("Battery: %dmV (%.1f%%)\n",
                  this->voltage,
                  device.get_device_state()->battery_percentage);
}