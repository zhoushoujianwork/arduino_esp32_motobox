#include "PowerManager.h"
#include "btn/BTN.h"
#include "config.h"

PowerManager::PowerManager() {
    // 设置默认值
    idleThreshold = 60000; // 默认1分钟无活动进入低功耗模式
    motionThreshold = 0.1; // 加速度变化阈值，根据实际调整
    lastMotionTime = 0;
    powerState = POWER_STATE_NORMAL;
    interruptRequested = false;
}

void PowerManager::begin() {
    Serial.println("PowerManager initialized");
    lastMotionTime = millis(); // 初始化时间
    powerState = POWER_STATE_NORMAL;
    interruptRequested = false;
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

#ifdef MODE_CLIENT
    // 客户端模式不使用睡眠功能
    return;
#endif

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
                
                // 记录当前设备状态
                Serial.printf("[电源管理] GPS状态: %s, WiFi状态: %s, MQTT状态: %s\n",
                    device.get_device_state()->gpsReady ? "已就绪" : "未就绪",
                    device.get_device_state()->wifiConnected ? "已连接" : "未连接",
                    device.get_device_state()->mqttConnected ? "已连接" : "未连接");
                
                // 使用浅睡眠代替深度睡眠
                enterLightSleepMode();
            }
        }
    }
}

// 配置运动检测中断
void PowerManager::configureMotionDetection() {
    Serial.println("[电源管理] 配置IMU运动检测中断...");
    
    // 使用IMU类的运动检测功能
    if (imu.enableMotionDetection(IMU_INT_PIN, motionThreshold)) {
        Serial.printf("[电源管理] IMU运动检测已配置，阈值: %.2f, 中断引脚: %d\n", 
                     motionThreshold, IMU_INT_PIN);
    } else {
        Serial.println("[电源管理] 警告: IMU运动检测配置失败");
    }
}

// 新增：打断低功耗模式进入过程
void PowerManager::interruptLowPowerMode() {
    // 设置打断请求标志
    interruptRequested = true;
    
    // 如果当前正处于任何睡眠相关状态，则立即更新状态
    if (powerState == POWER_STATE_COUNTDOWN || 
        powerState == POWER_STATE_PREPARING_SLEEP || 
        powerState == POWER_STATE_LIGHT_SLEEP) {
        
        Serial.println("[电源管理] 收到打断请求，取消进入低功耗模式");
        powerState = POWER_STATE_NORMAL;
        lastMotionTime = millis(); // 重置最后一次运动时间
        
        // 禁用IMU中断唤醒
        imu.disableMotionDetection();
        
        // 恢复屏幕亮度到最大
        #ifdef MODE_ALLINONE
        tft_set_brightness(255);
        Serial.println("[电源管理] 恢复屏幕亮度到最大");
        #endif
    }
}

bool PowerManager::detectMotion() {
    // 使用IMU数据检测运动
    imu_data_t* imuData = device.get_imu_data();
    
    // 计算加速度变化总量
    float accelMagnitude = sqrt(
        imuData->accel_x * imuData->accel_x + 
        imuData->accel_y * imuData->accel_y + 
        imuData->accel_z * imuData->accel_z
    );
    
    // 静止时加速度大约为1g (9.8m/s²)，检测变化
    static float lastAccelMagnitude = 0;
    float delta = abs(accelMagnitude - lastAccelMagnitude);
    lastAccelMagnitude = accelMagnitude;
    
    // 如果当前处于非正常状态，添加调试输出
    if (powerState != POWER_STATE_NORMAL) {
        static unsigned long lastDebugTime = 0;
        // 每500ms打印一次调试信息，避免输出过多
        if (millis() - lastDebugTime > 500) {
            Serial.printf("[电源管理] 加速度监测: 幅值=%.4f, 变化=%.4f, 阈值=%.4f\n", 
                          accelMagnitude, delta, motionThreshold);
            lastDebugTime = millis();
        }
    }
    
    // 降低运动检测阈值，提高灵敏度，简化为原先的1/2
    float adjustedThreshold = motionThreshold * 0.5;
    
    // 如果加速度变化大于阈值，认为有运动
    bool motionDetected = delta > adjustedThreshold;
    
    // 如果检测到运动且当前处于非正常工作状态，则自动打断低功耗进入过程
    if (motionDetected && powerState != POWER_STATE_NORMAL) {
        Serial.printf("[电源管理] 检测到运动 (幅值=%.4f, 变化=%.4f), 打断低功耗模式\n", 
                      accelMagnitude, delta);
        interruptLowPowerMode();
    }
    
    return motionDetected;
}

bool PowerManager::isDeviceIdle() {
    // 检查设备是否空闲足够长的时间
    return (millis() - lastMotionTime) > idleThreshold;
}

void PowerManager::disablePeripherals() {
    // 关闭不需要的外设以节省电量
    Serial.println("[电源管理] 正在关闭外设...");
    
    // 断开WiFi连接
#if defined(MODE_ALLINONE) || defined(MODE_SERVER)
    if (device.get_device_state()->wifiConnected) {
        Serial.println("[电源管理] 断开WiFi连接...");
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
        device.set_wifi_connected(false);
        
        // 断开MQTT连接
        if (device.get_device_state()->mqttConnected) {
            Serial.println("[电源管理] 断开MQTT连接...");
            device.set_mqtt_connected(false);
        }
        
        Serial.println("[电源管理] WiFi和MQTT已断开");
    }
#endif
    
    // 关闭GPS
#if defined(MODE_ALLINONE) || defined(MODE_SERVER)
    // 先让GPS模块进入待机模式，这样PPS信号灯也会关闭
    Serial.println("[电源管理] 使GPS模块进入待机模式...");
    gps.enterStandbyMode();
    
    // 挂起GPS任务
    extern TaskHandle_t gpsTaskHandle;
    if (gpsTaskHandle != NULL) {
        Serial.println("[电源管理] 挂起GPS任务...");
        vTaskSuspend(gpsTaskHandle);
    }
#endif

    // 关闭TFT显示
#ifdef MODE_ALLINONE
    // 使用新增的TFT睡眠函数
    Serial.println("[电源管理] 关闭显示屏...");
    tft_sleep();
#endif

    Serial.println("[电源管理] 所有外设已关闭");
}

void PowerManager::configureWakeupSources() {
    // 配置唤醒源
    Serial.println("[电源管理] 配置唤醒源...");
    
    // 使用定时器作为唤醒源，每60秒唤醒一次检查状态
    const uint64_t WAKEUP_INTERVAL_US = 60 * 1000000ULL;
    
    Serial.printf("[电源管理] 设置定时器唤醒：%llu 微秒后\n", WAKEUP_INTERVAL_US);
    esp_sleep_enable_timer_wakeup(WAKEUP_INTERVAL_US);
    
    // 新增：配置按钮唤醒
    #if defined(MODE_ALLINONE) || defined(MODE_SERVER)
    // 检查按钮引脚是否为有效的RTC GPIO (ESP32-S3只有GPIO0-GPIO21是RTC GPIO)
    if (BTN_PIN >= 0 && BTN_PIN <= 21) {
        Serial.printf("[电源管理] 设置按钮唤醒 (GPIO%d)\n", BTN_PIN);
        
        // 检查按钮当前状态，避免无限重启
        extern BTN button;
        if (button.isPressed()) {
            Serial.println("[电源管理] 警告：按钮当前处于按下状态，跳过按钮唤醒配置");
        } else {
            // 初始化RTC GPIO
            rtc_gpio_init((gpio_num_t)BTN_PIN);
            // 设置GPIO模式为输入
            rtc_gpio_set_direction((gpio_num_t)BTN_PIN, RTC_GPIO_MODE_INPUT_ONLY);
            // 启用内部上拉电阻，确保按钮未按下时为高电平
            rtc_gpio_pullup_en((gpio_num_t)BTN_PIN);
            rtc_gpio_pulldown_dis((gpio_num_t)BTN_PIN);
            // 配置为低电平触发唤醒
            esp_sleep_enable_ext0_wakeup((gpio_num_t)BTN_PIN, 0); 
            
            // 再次检查GPIO状态，如果已经是低电平，则禁用此唤醒源
            if (rtc_gpio_get_level((gpio_num_t)BTN_PIN) == 0) {
                Serial.println("[电源管理] 警告：按钮引脚处于低电平，禁用按钮唤醒源");
                esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_EXT0);
            }
        }
    } else {
        Serial.printf("[电源管理] 警告：BTN_PIN (GPIO%d) 不是有效的RTC GPIO，无法用作睡眠唤醒源\n", BTN_PIN);
        Serial.println("[电源管理] ESP32-S3只有GPIO0-GPIO21能用作RTC GPIO");
        Serial.println("[电源管理] 建议：修改硬件连接，将按钮接到GPIO0-GPIO21之间的任一引脚，并更新config.h中的BTN_PIN定义");
    }
    #endif
}

// 新增：进入浅睡眠模式
void PowerManager::enterLightSleepMode() {
#ifdef MODE_CLIENT
    // 客户端模式不使用睡眠功能
    Serial.println("[电源管理] 客户端模式不使用睡眠功能");
    return;
#endif

    Serial.println("[电源管理] 正在进入浅睡眠模式...");
    
    // 设置电源状态为倒计时
    powerState = POWER_STATE_COUNTDOWN;
    
    // 倒计时总时间（秒）
    const int countdownTime = 5; // 缩短倒计时
    
    // 添加UI提示
    #ifdef MODE_ALLINONE
    // 保存当前亮度设置
    static const int maxBrightness = 255;
    #endif
    
    // 添加5秒倒计时
    Serial.println("[电源管理] 5秒倒计时开始，如有动作或按钮按下将取消进入浅睡眠模式...");
    
    for (int i = countdownTime; i > 0; i--) {
        Serial.printf("[电源管理] 倒计时: %d 秒\n", i);
        
        // 设置屏幕亮度，逐渐降低
        #ifdef MODE_ALLINONE
        int brightness = map(i, countdownTime, 1, maxBrightness, maxBrightness/5);
        tft_set_brightness(brightness);
        Serial.printf("[电源管理] 屏幕亮度设置为 %d/%d\n", brightness, maxBrightness);
        #endif
        
        // 使用更小的时间间隔检查中断请求，提高响应速度
        unsigned long countdownStartTime = millis();
        while (millis() - countdownStartTime < 1000) { // 每秒分为多个小间隔检查
            // 检查是否已请求打断
            if (interruptRequested) {
                Serial.println("[电源管理] 收到打断请求，取消进入浅睡眠模式");
                interruptRequested = false;
                powerState = POWER_STATE_NORMAL;
                
                // 恢复屏幕亮度到最大
                #ifdef MODE_ALLINONE
                tft_set_brightness(maxBrightness);
                Serial.println("[电源管理] 恢复屏幕亮度到最大");
                #endif
                
                return; // 退出函数，不进入浅睡眠模式
            }
            
            // 在倒计时期间检测运动，如果有运动则取消进入浅睡眠模式
            if (detectMotion()) {
                Serial.println("[电源管理] 检测到运动，取消进入浅睡眠模式");
                lastMotionTime = millis(); // 更新最后一次运动时间
                powerState = POWER_STATE_NORMAL;
                
                // 恢复屏幕亮度到最大
                #ifdef MODE_ALLINONE
                tft_set_brightness(maxBrightness);
                Serial.println("[电源管理] 恢复屏幕亮度到最大");
                #endif
                
                return; // 退出函数，不进入浅睡眠模式
            }
            
            delay(50); // 小间隔检查，更快响应
        }
    }
    
    // 设置电源状态为准备睡眠
    powerState = POWER_STATE_PREPARING_SLEEP;
    
    // 再次检查是否已请求打断
    if (interruptRequested) {
        Serial.println("[电源管理] 收到打断请求，取消进入浅睡眠模式");
        interruptRequested = false;
        powerState = POWER_STATE_NORMAL;
        
        // 恢复屏幕亮度到最大
        #ifdef MODE_ALLINONE
        tft_set_brightness(maxBrightness);
        Serial.println("[电源管理] 恢复屏幕亮度到最大");
        #endif
        
        return; // 退出函数，不进入浅睡眠模式
    }
    
    Serial.println("[电源管理] 倒计时结束，开始关闭外设...");
    
    // 关闭外设
    disablePeripherals();
    
    // 最后一次检查是否有打断请求
    if (interruptRequested) {
        Serial.println("[电源管理] 收到打断请求，取消进入浅睡眠模式");
        interruptRequested = false;
        powerState = POWER_STATE_NORMAL;
        
        // 恢复屏幕亮度到最大
        #ifdef MODE_ALLINONE
        tft_set_brightness(maxBrightness);
        Serial.println("[电源管理] 恢复屏幕亮度到最大");
        #endif
        
        return; // 退出函数，不进入浅睡眠模式
    }
    
    // 配置唤醒源
    // 不使用定时唤醒，只靠运动唤醒
    Serial.println("[电源管理] 配置运动唤醒...");
    
    // 配置IMU中断检测运动
    configureMotionDetection();

    // 配置RTC GPIO中断唤醒
    if (IMU_INT_PIN >= 0 && IMU_INT_PIN <= 21) {
        Serial.printf("[电源管理] 设置IMU中断引脚唤醒 (GPIO%d)\n", IMU_INT_PIN);
        
        // 初始化RTC GPIO
        rtc_gpio_init((gpio_num_t)IMU_INT_PIN);
        // 设置GPIO模式为输入
        rtc_gpio_set_direction((gpio_num_t)IMU_INT_PIN, RTC_GPIO_MODE_INPUT_ONLY);
        // 启用内部上拉电阻
        rtc_gpio_pullup_en((gpio_num_t)IMU_INT_PIN);
        rtc_gpio_pulldown_dis((gpio_num_t)IMU_INT_PIN);
        // 配置为高电平触发唤醒（根据IMU中断的实际输出）
        esp_sleep_enable_ext0_wakeup((gpio_num_t)IMU_INT_PIN, 1);
    } else {
        Serial.printf("[电源管理] 警告：IMU_INT_PIN (GPIO%d) 不是有效的RTC GPIO\n", IMU_INT_PIN);
        // 设置60秒定时唤醒作为备份
        const uint64_t WAKEUP_INTERVAL_US = 60 * 1000000ULL;
        esp_sleep_enable_timer_wakeup(WAKEUP_INTERVAL_US);
    }
    
    // 配置按钮唤醒
    #if defined(MODE_ALLINONE) || defined(MODE_SERVER)
    if (BTN_PIN >= 0 && BTN_PIN <= 21) {
        Serial.printf("[电源管理] 设置按钮唤醒 (GPIO%d)\n", BTN_PIN);
        
        // 检查按钮当前状态，避免无限重启
        extern BTN button;
        if (button.isPressed()) {
            Serial.println("[电源管理] 警告：按钮当前处于按下状态，跳过按钮唤醒配置");
        } else {
            // 初始化RTC GPIO
            rtc_gpio_init((gpio_num_t)BTN_PIN);
            // 设置GPIO模式为输入
            rtc_gpio_set_direction((gpio_num_t)BTN_PIN, RTC_GPIO_MODE_INPUT_ONLY);
            // 启用内部上拉电阻，确保按钮未按下时为高电平
            rtc_gpio_pullup_en((gpio_num_t)BTN_PIN);
            rtc_gpio_pulldown_dis((gpio_num_t)BTN_PIN);
            // 配置为低电平触发唤醒
            esp_sleep_enable_ext1_wakeup(1ULL << BTN_PIN, ESP_EXT1_WAKEUP_ALL_LOW);
        }
    }
    #endif
    
    // 打印设备信息和统计数据
    Serial.println("[电源管理] 设备状态汇总:");
    Serial.printf("[电源管理] 设备ID: %s\n", device.get_device_id().c_str());
    Serial.printf("[电源管理] 本次运行时间: %lu 毫秒\n", millis());
    
    // 延迟一段时间以允许串行输出完成
    delay(500);
    
    // 设置为浅睡眠模式
    powerState = POWER_STATE_LIGHT_SLEEP;
    
    // 进入浅睡眠模式
    Serial.println("[电源管理] 正在进入浅睡眠模式...");
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
    esp_light_sleep_start();
    
    // 唤醒后的处理
    Serial.println("[电源管理] 从浅睡眠模式唤醒");
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    
    switch(wakeup_reason) {
        case ESP_SLEEP_WAKEUP_EXT0:
            Serial.println("[电源管理] 通过IMU运动检测唤醒");
            break;
        case ESP_SLEEP_WAKEUP_EXT1:
            Serial.println("[电源管理] 通过按钮唤醒");
            break;
        case ESP_SLEEP_WAKEUP_TIMER:
            Serial.println("[电源管理] 通过定时器唤醒");
            break;
        default:
            Serial.printf("[电源管理] 未知原因唤醒，代码: %d\n", wakeup_reason);
    }
    
    // 清除IMU中断标志
    imu.disableMotionDetection();
    
    // 唤醒外设
    #ifdef MODE_ALLINONE
    // 恢复显示屏
    tft_wakeup();
    tft_set_brightness(255);
    #endif
    
    // 重置状态
    powerState = POWER_STATE_NORMAL;
    lastMotionTime = millis();
    
    Serial.println("[电源管理] 已完全唤醒，恢复正常工作");
}

void PowerManager::enterLowPowerMode() {
#ifdef MODE_CLIENT
    // 客户端模式不使用睡眠功能
    Serial.println("[电源管理] 客户端模式不使用睡眠功能");
    return;
#endif

    Serial.println("[电源管理] 浅睡眠模式已启用，改为使用浅睡眠模式");
    enterLightSleepMode();
} 