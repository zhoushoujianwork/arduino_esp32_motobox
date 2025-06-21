#include "BAT.h"
#include "config.h"

BAT bat(BAT_PIN, CHARGING_STATUS_PIN);

BAT::BAT(int adc_pin, int charging_pin)
    : pin(adc_pin),
      charging_pin(charging_pin),
      _is_charging(false),
      min_voltage(2800),
      max_voltage(4200),
      buffer_index(0),
      voltage(0),
      ema_voltage(0),
      last_output_voltage(0),
      stable_voltage(0),
      output_counter(0),
      observed_max(0),
      observed_min(5000),
      last_calibration(0),
      stable_count(0),
      _debug(false)
{
    loadCalibration();  // 加载已保存的校准数据
}

void BAT::debugPrint(const String& message) {
    if (!_debug) return;
    
    unsigned long currentTime = millis();
    Serial.print("[BAT] [debug] [");
    Serial.print(currentTime);
    Serial.print("ms] ");
    Serial.println(message);
}

void BAT::begin()
{
    setDebug(true);

    pinMode(pin, INPUT);
    pinMode(charging_pin, INPUT_PULLUP); // 初始化充电检测引脚

    // 初始化缓冲区
    memset(voltage_buffer, 0, sizeof(voltage_buffer));
    
    String debug_msg = "初始化开始\n";
    debug_msg += "配置 -> 引脚: " + String(pin) + ", ADC通道: " + String(analogGetChannel(pin)) + "\n";
    debug_msg += "参数 -> 最小电压: " + String(min_voltage) + "mV, 最大电压: " + String(max_voltage) + "mV\n";
    debug_msg += "滤波 -> 窗口大小: " + String(FILTER_WINDOW_SIZE) + ", EMA系数: " + String(EMA_ALPHA, 3) + "\n";
    debug_msg += "校准 -> 间隔: " + String(CALIBRATION_INTERVAL) + "ms, 稳定阈值: " + String(STABLE_COUNT);
    debugPrint(debug_msg);
}   

void BAT::loadCalibration()
{
    PreferencesUtils::loadBatteryRange(min_voltage, max_voltage);
    observed_min = min_voltage;
    observed_max = max_voltage;
    debugPrint("加载校准数据 -> min: " + String(min_voltage) + "mV, max: " + String(max_voltage) + "mV");
}

void BAT::saveCalibration()
{
    PreferencesUtils::saveBatteryRange(observed_min, observed_max);
    String debug_msg = "保存校准数据 -> min: " + String(observed_min) + "mV, max: " + String(observed_max) + "mV";
    debug_msg += " [变化: " + String(observed_min - min_voltage) + "mV, " + String(observed_max - max_voltage) + "mV]";
    debugPrint(debug_msg);
}

void BAT::updateCalibration()
{
    if (stable_voltage < VOLTAGE_MIN_LIMIT || stable_voltage > VOLTAGE_MAX_LIMIT) {
        debugPrint("电压超出校准范围: " + String(stable_voltage) + "mV [" + 
                   String(VOLTAGE_MIN_LIMIT) + "-" + String(VOLTAGE_MAX_LIMIT) + "]");
        return;
    }

    bool changed = false;
    
    // 更新观察到的最大/最小值
    if (stable_voltage > observed_max) {
        debugPrint("发现新的最大电压: " + String(stable_voltage) + "mV (原: " + String(observed_max) + "mV)");
        observed_max = stable_voltage;
        changed = true;
    }
    if (stable_voltage < observed_min) {
        debugPrint("发现新的最小电压: " + String(stable_voltage) + "mV (原: " + String(observed_min) + "mV)");
        observed_min = stable_voltage;
        changed = true;
    }

    // 如果电压稳定且超过校准间隔，更新校准参数
    unsigned long now = millis();
    if (changed) {
        stable_count = 0;  // 重置稳定计数
    } else {
        stable_count++;
        if (stable_count >= STABLE_COUNT && 
            (now - last_calibration) > CALIBRATION_INTERVAL) {
            // 更新校准参数
            if (abs(observed_max - max_voltage) > 50 || 
                abs(observed_min - min_voltage) > 50) {
                String debug_msg = "校准更新 -> 稳定计数: " + String(stable_count);
                debug_msg += ", 间隔: " + String(now - last_calibration) + "ms";
                debugPrint(debug_msg);
                
                min_voltage = observed_min;
                max_voltage = observed_max;
                saveCalibration();
                last_calibration = now;
            }
        }
    }
}

void BAT::loop()
{
    // 更新充电状态
    _is_charging = (digitalRead(charging_pin) == LOW);
    device_state.is_charging = _is_charging;

    static constexpr float VOLTAGE_MULTIPLIER = 2.0f; // 电压倍数常量
    static constexpr int ADC_MAX_VALUE = 4095;        // ADC最大值
    static int last_percentage = -1;                  // 上一次显示的百分比

    // 多次采样取平均，进一步抑制ADC抖动
    int sum_analogVolts = 0;
    const int ADC_SAMPLE_COUNT = 4;
    for (int i = 0; i < ADC_SAMPLE_COUNT; ++i) {
        sum_analogVolts += analogReadMilliVolts(this->pin);
        delay(2); // 适当延时
    }
    int analogVolts = sum_analogVolts / ADC_SAMPLE_COUNT;
    int current_voltage = analogVolts * VOLTAGE_MULTIPLIER;

    // 1. 滑动窗口平均滤波 - 使用运行总和优化计算
    static int running_sum = 0;

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
        running_sum -= voltage_buffer[buffer_index];
        running_sum += current_voltage;
        voltage_buffer[buffer_index] = current_voltage;
        buffer_index = (buffer_index + 1) % FILTER_WINDOW_SIZE;
        voltage = running_sum / FILTER_WINDOW_SIZE;
    }

    // 2. 指数移动平均滤波 (EMA)
    if (ema_voltage == 0)
    {
        ema_voltage = voltage;
    }
    else
    {
        int voltage_diff = abs(voltage - ema_voltage);
        float alpha = EMA_ALPHA;
        if (voltage_diff > MAX_VOLTAGE_JUMP)
        {
            alpha = EMA_ALPHA * 0.5f;
        }
        ema_voltage = (int)(alpha * voltage + (1.0f - alpha) * ema_voltage);
    }

    // 3. 输出分频 - 每N次计算才更新一次输出值
    output_counter = (output_counter + 1) % OUTPUT_DIVIDER;
    if (output_counter == 0)
    {
        int output_diff = abs(ema_voltage - last_output_voltage);
        if (output_diff > 5 || last_output_voltage == 0)
        {
            last_output_voltage = ema_voltage;
        }
        stable_voltage = last_output_voltage;
    }

    // 更新校准
    updateCalibration();

    // 使用新的百分比计算方法
    int percentage = calculatePercentage(stable_voltage);
    percentage = constrain(percentage, 0, 100);

    // 只有当百分比变化超过1%才更新
    if (abs(percentage - last_percentage) >= 1 || last_percentage == -1) {
        device_state.battery_voltage = stable_voltage;
        device_state.battery_percentage = percentage;
        last_percentage = percentage;
        
        String debug_msg = "状态更新 -> 充电: " + String(_is_charging ? "是" : "否");
        debug_msg += ", 电压: " + String(stable_voltage) + "mV (" + String(percentage) + "%)";
        debug_msg += ", 范围: " + String(min_voltage) + "-" + String(max_voltage) + "mV";
        debug_msg += "\n滤波详情 -> 原始: " + String(current_voltage) + "mV";
        debug_msg += ", 窗口: " + String(voltage) + "mV";
        debug_msg += ", EMA: " + String(ema_voltage) + "mV";
        debug_msg += ", 稳定: " + String(stable_voltage) + "mV";
        debugPrint(debug_msg);
    }
}

void BAT::print_voltage()
{
    String debug_msg = "电压报告 -> " + String(stable_voltage) + "mV";
    debug_msg += " (" + String(device_state.battery_percentage) + "%)";
    debug_msg += ", 充电: " + String(device_state.is_charging ? "是" : "否");
    Serial.println(debug_msg);
}

const int BAT::VOLTAGE_LEVELS[] = {4200, 4000, 3800, 3600, 3400, 3200, 3000, 2800};
const int BAT::PERCENT_LEVELS[] = {100,   90,   80,   60,   45,   30,   15,    0};
const int BAT::LEVEL_COUNT = 8;  // 数组大小固定为8

int BAT::calculatePercentage(int voltage) {
    if (voltage >= VOLTAGE_LEVELS[0]) return 100;
    if (voltage <= VOLTAGE_LEVELS[LEVEL_COUNT-1]) return 0;
    
    // 查找电压所在区间
    for (int i = 0; i < LEVEL_COUNT-1; i++) {
        if (voltage <= VOLTAGE_LEVELS[i] && voltage > VOLTAGE_LEVELS[i+1]) {
            // 线性插值计算百分比
            return map(voltage, 
                      VOLTAGE_LEVELS[i+1], VOLTAGE_LEVELS[i],
                      PERCENT_LEVELS[i+1], PERCENT_LEVELS[i]);
        }
    }
    return 0;
}

// 新增实现
bool BAT::isCharging() {
    return _is_charging;
}