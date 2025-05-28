#include "PowerManager.h"
#include "btn/BTN.h"
#include "esp_wifi.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"

// 初始化静态变量
#if ENABLE_SLEEP
RTC_DATA_ATTR bool PowerManager::sleepEnabled = true;  // 默认启用休眠功能
#else
RTC_DATA_ATTR bool PowerManager::sleepEnabled = false; // 默认禁用休眠功能
#endif

PowerManager::PowerManager() {
    // 设置默认值
    idleThreshold = 10000; // 默认1分钟无活动进入低功耗模式
    motionThreshold = 0.1; // 加速度变化阈值，根据实际调整
    lastMotionTime = 0;
    powerState = POWER_STATE_NORMAL;
    interruptRequested = false;
    
    // 初始化运动检测相关变量
    lastAccelMagnitude = 0;
    accumulatedDelta = 0;
    sampleIndex = 0;
}

void PowerManager::begin() {
    lastMotionTime = millis();
    powerState = POWER_STATE_NORMAL;
    interruptRequested = false;
    
    // 处理唤醒事件
    handleWakeup();
    
    Serial.println("[电源管理] 休眠功能已启用 (编译时配置)");
}

void PowerManager::handleWakeup() {
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    
    switch(wakeup_reason) {
        case ESP_SLEEP_WAKEUP_EXT0: {
            if (IMU_INT_PIN >= 0 && IMU_INT_PIN <= 21) {
                // 等待引脚状态稳定
                delay(50);
                int pinState = digitalRead(IMU_INT_PIN);
                Serial.printf("[电源管理] IMU引脚状态: %d\n", pinState);
                
                // 验证是否为真实的WakeOnMotion事件
                extern IMU imu;
                Serial.println("[电源管理] IMU运动唤醒检测到，记录运动事件");
                
                // 从WakeOnMotion模式恢复到正常模式
                imu.restoreFromDeepSleep();
                
                // 检查是否为WakeOnMotion事件
                if (imu.checkWakeOnMotionEvent()) {
                    Serial.println("[电源管理] 确认为WakeOnMotion事件唤醒");
                } else {
                    Serial.println("[电源管理] 可能为其他原因唤醒");
                }
            }
            break;
        }
        case ESP_SLEEP_WAKEUP_TIMER:
            Serial.println("[电源管理] 定时器唤醒");
            // 这里可以添加定时唤醒后的特殊处理
            break;
        default:
            if (wakeup_reason != ESP_SLEEP_WAKEUP_UNDEFINED) {
                Serial.printf("[电源管理] 其他唤醒原因: %d\n", wakeup_reason);
            }
            break;
    }
}

void PowerManager::configurePowerDomains() {
    // 配置ESP32-S3特定的省电选项
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);  // 保持RTC外设供电
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_ON);  // 保持RTC慢速内存
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);  // 关闭RTC快速内存
    esp_sleep_pd_config(ESP_PD_DOMAIN_XTAL, ESP_PD_OPTION_OFF);  // 关闭XTAL
}

void PowerManager::configureWakeupSources() {
    // 配置IMU运动唤醒
    if (IMU_INT_PIN >= 0 && IMU_INT_PIN <= 21) {  // 确保是有效的RTC GPIO
        // 初始化RTC GPIO
        rtc_gpio_init((gpio_num_t)IMU_INT_PIN);
        rtc_gpio_set_direction((gpio_num_t)IMU_INT_PIN, RTC_GPIO_MODE_INPUT_ONLY);
        rtc_gpio_pullup_en((gpio_num_t)IMU_INT_PIN);    // 使用上拉电阻
        rtc_gpio_pulldown_dis((gpio_num_t)IMU_INT_PIN); // 禁用下拉
        
        // WakeOnMotion模式中，官方例子建议检查默认引脚值来确定触发方式
        // 由于我们在configWakeOnMotion中设置defaultPinValue=1，所以检测低电平触发
        esp_sleep_enable_ext0_wakeup((gpio_num_t)IMU_INT_PIN, 0);  // 低电平触发
        
        // 配置IMU的运动检测中断
        extern IMU imu;
        imu.configureForDeepSleep();  // 使用新的WakeOnMotion配置
        
        // 添加延迟确保IMU配置稳定
        delay(100);
        
        Serial.printf("[电源管理] IMU WakeOnMotion唤醒源配置完成 (GPIO%d, 低电平触发)\n", IMU_INT_PIN);
    }
    
    // 配置定时器唤醒作为备份
    const uint64_t WAKEUP_INTERVAL_US = 30 * 60 * 1000000ULL; // 30分钟
    esp_sleep_enable_timer_wakeup(WAKEUP_INTERVAL_US);
    Serial.println("[电源管理] 定时器唤醒源配置完成");
}

void PowerManager::disablePeripherals() {
    Serial.println("[电源管理] 正在关闭外设...");
    
    // WiFi关闭优化
    if (device.get_device_state()->wifiConnected) {
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
        esp_wifi_deinit();  // 使用deinit替代stop
        device.set_wifi_connected(false);
        Serial.println("[电源管理] WiFi已关闭");
    }
    
    // 蓝牙关闭
    btStop();
    Serial.println("[电源管理] 蓝牙已关闭");
    
    // ADC关闭
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_11);
    adc1_ulp_enable();  // 使用ULP模式替代完全关闭
    Serial.println("[电源管理] ADC已配置为低功耗模式");
    
    // TFT显示屏优化关闭
    #ifdef MODE_ALLINONE
    extern void tft_sleep();  // 声明外部函数
    tft_sleep();  // 调用TFT睡眠函数
    Serial.println("[电源管理] 显示屏已进入睡眠模式");
    #endif
    
    // GPS关闭优化
    #if defined(MODE_ALLINONE) || defined(MODE_SERVER)
    extern TaskHandle_t gpsTaskHandle;
    if (gpsTaskHandle != NULL) {
        vTaskDelete(gpsTaskHandle);
        gpsTaskHandle = NULL;
        Serial.println("[电源管理] GPS任务已停止");
    }
    #endif
    
    // IMU配置优化
    #if defined(MODE_ALLINONE) || defined(MODE_SERVER)
    extern IMU imu;
    imu.configureForDeepSleep();  // 使用新的低功耗配置方法
    Serial.println("[电源管理] IMU已配置为低功耗模式");
    #endif
    
    Serial.println("[电源管理] 所有外设已关闭");
}

bool PowerManager::detectMotion() {
    extern IMU imu;
    imu_data_t* imuData = device.get_imu_data();
    
    // 计算当前加速度幅值
    float accelMagnitude = sqrt(
        imuData->accel_x * imuData->accel_x + 
        imuData->accel_y * imuData->accel_y + 
        imuData->accel_z * imuData->accel_z
    );
    
    // 计算变化量
    float delta = abs(accelMagnitude - lastAccelMagnitude);
    lastAccelMagnitude = accelMagnitude;
    
    // 累积变化量
    accumulatedDelta += delta;
    sampleIndex++;
    
    // 每收集SAMPLE_COUNT个样本进行一次判断
    if (sampleIndex >= SAMPLE_COUNT) {
        float averageDelta = accumulatedDelta / SAMPLE_COUNT;
        bool motionDetected = averageDelta > (motionThreshold * 0.8);
        
        // 重置累积值
        accumulatedDelta = 0;
        sampleIndex = 0;
        
        // 调试输出
        if (powerState != POWER_STATE_NORMAL) {
            Serial.printf("[电源管理] 运动检测: 平均变化=%.4f, 阈值=%.4f\n", 
                        averageDelta, motionThreshold * 0.8);
        }
        
        // 如果检测到运动且当前处于非正常工作状态，则自动打断低功耗进入过程
        if (motionDetected && powerState != POWER_STATE_NORMAL) {
            Serial.printf("[电源管理] 检测到运动 (平均变化=%.4f), 打断低功耗模式\n", averageDelta);
            interruptLowPowerMode();
        }
        
        return motionDetected;
    }
    
    return false;
}

void PowerManager::loop() {
    // 如果已请求打断低功耗模式，则在此处理
    if (interruptRequested) {
        if (powerState != POWER_STATE_NORMAL) {
            Serial.println("[电源管理] 低功耗模式已被打断");
            powerState = POWER_STATE_NORMAL;
        }
        interruptRequested = false;
        lastMotionTime = millis(); // 重置最后一次运动时间
        return;
    }

#if !ENABLE_SLEEP
    // 如果编译时禁用了休眠功能，始终保持在正常状态
    if (powerState != POWER_STATE_NORMAL) {
        powerState = POWER_STATE_NORMAL;
    }
    return;
#else
    // 编译时启用了休眠功能，但需要检查运行时状态
    if (!sleepEnabled) {
        // 保持在正常状态，不进入休眠
        if (powerState != POWER_STATE_NORMAL) {
            powerState = POWER_STATE_NORMAL;
        }
        return;
    }

    // 仅在正常工作状态下检测运动和空闲状态
    if (powerState == POWER_STATE_NORMAL) {
        // 检测设备是否有运动
        if (detectMotion()) {
            // 如果有运动，更新最后一次运动时间
            lastMotionTime = millis();
        } else {
            // 检查设备是否空闲足够长的时间
            if (isDeviceIdle()) {
                Serial.println("[电源管理] 设备已静止超过1分钟，准备进入低功耗模式...");
                Serial.printf("[电源管理] 电池状态: %d%%, 电压: %dmV\n", 
                    device.get_device_state()->battery_percentage,
                    device.get_device_state()->battery_voltage);
                enterLowPowerMode();
            }
        }
    }
#endif
}

// 新增：打断低功耗模式进入过程
void PowerManager::interruptLowPowerMode() {
    // 设置打断请求标志
    interruptRequested = true;
    
    // 如果当前正处于倒计时状态，则立即更新状态
    if (powerState == POWER_STATE_COUNTDOWN || powerState == POWER_STATE_PREPARING_SLEEP) {
        Serial.println("[电源管理] 收到打断请求，取消进入低功耗模式");
        powerState = POWER_STATE_NORMAL;
        lastMotionTime = millis(); // 重置最后一次运动时间
        
        // 恢复屏幕亮度到最大
        #ifdef MODE_ALLINONE
        tft_set_brightness(255);
        Serial.println("[电源管理] 恢复屏幕亮度到最大");
        #endif
    }
}

bool PowerManager::isDeviceIdle() {
    // 检查设备是否空闲足够长的时间
    return (millis() - lastMotionTime) > idleThreshold;
}

void PowerManager::enterLowPowerMode() {
#ifdef MODE_CLIENT
    // 客户端模式不使用睡眠功能
    Serial.println("[电源管理] 客户端模式不使用睡眠功能");
    return;
#endif

#if !ENABLE_SLEEP
    // 编译时禁用了休眠功能
    Serial.println("[电源管理] 休眠功能已在编译时禁用，不进入低功耗模式");
    return;
#else
    // 编译时启用了休眠功能，但需要检查运行时状态
    if (!sleepEnabled) {
        Serial.println("[电源管理] 休眠功能已禁用，不进入低功耗模式");
        return;
    }

    Serial.println("[电源管理] 正在进入低功耗模式...");
    
    // 设置电源状态为倒计时
    powerState = POWER_STATE_COUNTDOWN;
    
    // 倒计时总时间（秒）
    const int countdownTime = 10;
    
    // 添加UI提示
    #ifdef MODE_ALLINONE
    // 保存当前亮度设置
    static const int maxBrightness = 255;
    #endif
    
    // 添加10秒倒计时
    Serial.println("[电源管理] 10秒倒计时开始，如有动作或按钮按下将取消进入低功耗模式...");
    
    for (int i = countdownTime; i > 0; i--) {
        Serial.printf("[电源管理] 倒计时: %d 秒\n", i);
        
        // 设置屏幕亮度，逐渐降低
        #ifdef MODE_ALLINONE
        int brightness = map(i, countdownTime, 1, maxBrightness, maxBrightness/10);
        tft_set_brightness(brightness);
        Serial.printf("[电源管理] 屏幕亮度设置为 %d/%d\n", brightness, maxBrightness);
        #endif
        
        // 使用更小的时间间隔检查中断请求，提高响应速度
        unsigned long countdownStartTime = millis();
        while (millis() - countdownStartTime < 1000) { // 每秒分为多个小间隔检查
            // 检查是否已请求打断
            if (interruptRequested) {
                Serial.println("[电源管理] 收到打断请求，取消进入低功耗模式");
                interruptRequested = false;
                powerState = POWER_STATE_NORMAL;
                
                // 恢复屏幕亮度到最大
                #ifdef MODE_ALLINONE
                tft_set_brightness(maxBrightness);
                Serial.println("[电源管理] 恢复屏幕亮度到最大");
                #endif
                
                return; // 退出函数，不进入低功耗模式
            }
            
            // 在倒计时期间检测运动，如果有运动则取消进入低功耗模式
            if (detectMotion()) {
                Serial.println("[电源管理] 检测到运动，取消进入低功耗模式");
                lastMotionTime = millis(); // 更新最后一次运动时间
                powerState = POWER_STATE_NORMAL;
                
                // 恢复屏幕亮度到最大
                #ifdef MODE_ALLINONE
                tft_set_brightness(maxBrightness);
                Serial.println("[电源管理] 恢复屏幕亮度到最大");
                #endif
                
                return; // 退出函数，不进入低功耗模式
            }
            
            delay(50); // 小间隔检查，更快响应
        }
    }
    
    // 设置电源状态为准备睡眠
    powerState = POWER_STATE_PREPARING_SLEEP;
    
    // 再次检查是否已请求打断
    if (interruptRequested) {
        Serial.println("[电源管理] 收到打断请求，取消进入低功耗模式");
        interruptRequested = false;
        powerState = POWER_STATE_NORMAL;
        
        // 恢复屏幕亮度到最大
        #ifdef MODE_ALLINONE
        tft_set_brightness(maxBrightness);
        Serial.println("[电源管理] 恢复屏幕亮度到最大");
        #endif
        
        return; // 退出函数，不进入低功耗模式
    }
    
    Serial.println("[电源管理] 倒计时结束，开始关闭外设...");
    
    // 在进入深度睡眠前保存关键状态到RTC内存
    RTC_DATA_ATTR static uint32_t sleepCount = 0;
    sleepCount++;
    
    // 优化外设关闭顺序
    disablePeripherals();
    
    // 配置电源域
    configurePowerDomains();
    
    // 配置唤醒源
    configureWakeupSources();
    
    // 打印设备信息和统计数据
    Serial.println("[电源管理] 设备状态汇总:");
    Serial.printf("[电源管理] 设备ID: %s\n", device.get_device_id().c_str());
    Serial.printf("[电源管理] 本次运行时间: %lu 毫秒\n", millis());
    Serial.printf("[电源管理] 累计睡眠次数: %d\n", sleepCount);
    
    // 延迟一段时间以允许串口输出完成
    Serial.flush();
    delay(100);
    
    // 进入深度睡眠模式
    Serial.println("[电源管理] 正在进入深度睡眠模式...");
    esp_deep_sleep_start();
#endif
}

bool PowerManager::setupIMUWakeupSource(int intPin, float threshold) {
    if (intPin < 0 || intPin > 21) return false;
    
    extern IMU imu;
    if (!imu.enableMotionDetection(intPin, threshold)) {
        Serial.println("[电源管理] IMU运动检测配置失败");
        return false;
    }
    
    Serial.printf("[电源管理] IMU运动检测已配置: GPIO%d, 阈值=%.3fg\n", intPin, threshold);
    return true;
}
