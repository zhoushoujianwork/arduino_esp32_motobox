#include "BAT.h"

// 3300, 4200
BAT::BAT(int pin, int min_voltage, int max_voltage)
{
    this->pin = pin;
    this->min_voltage = min_voltage;
    this->max_voltage = max_voltage;

    // 初始化滤波缓冲区
    for (int i = 0; i < FILTER_WINDOW_SIZE; i++)
    {
        this->voltage_buffer[i] = 0;
    }
    this->buffer_index = 0;
}

void BAT::loop()
{
    // int analogValue = analogRead(this->pin);
    // Serial.printf("ADC analog value = %d volts = %d\n", analogValue, analogValue * 2.6 / 4095.0);

    int analogVolts = analogReadMilliVolts(this->pin);
    int current_voltage = analogVolts * 2;

    // 更新滤波缓冲区
    this->voltage_buffer[this->buffer_index] = current_voltage;
    this->buffer_index = (this->buffer_index + 1) % FILTER_WINDOW_SIZE;

    // 计算平均值
    int sum = 0;
    for (int i = 0; i < FILTER_WINDOW_SIZE; i++)
    {
        sum += this->voltage_buffer[i];
    }
    this->voltage = sum / FILTER_WINDOW_SIZE;
}

int BAT::getPercentage()
{
    int percentage = map(this->voltage, this->min_voltage, this->max_voltage, 0, 100);
    if (percentage > 100)
    {
        percentage = 100;
    }
    else if (percentage < 0)
    {
        percentage = 0;
    }

    return percentage;
}

void BAT::print_voltage()
{
    Serial.printf("voltage = %d\t", this->voltage);
    Serial.printf("percentage = %d\t", this->getPercentage());
    Serial.println();
}