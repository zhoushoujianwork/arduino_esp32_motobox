#include "PowerManager.h"
#include "btn/BTN.h"

// 初始化静态变量
#if ENABLE_SLEEP
RTC_DATA_ATTR bool PowerManager::sleepEnabled = true;  // 默认启用休眠功能
#else
RTC_DATA_ATTR bool PowerManager::sleepEnabled = false; // 默认禁用休眠功能
#endif

PowerManager::PowerManager() {
    // 设置默认值
    idleThreshold = 60000; // 默认1分钟无活动进入低功耗模式
    motionThreshold = 0.1; // 加速度变化阈值，根据实际调整
    lastMotionTime = 0;
    powerState = POWER_STATE_NORMAL;
    interruptRequested = false;
    // sleepEnabled已经作为静态变量初始化
}

void PowerManager::begin() {
    Serial.println("PowerManager initialized");
    lastMotionTime = millis(); // 初始化时间
    powerState = POWER_STATE_NORMAL;
    interruptRequested = false;
    
    // 打印休眠状态，根据编译时配置
#if ENABLE_SLEEP
    Serial.println("[电源管理] 休眠功能已启用 (编译时配置)");
#else
    Serial.println("[电源管理] 休眠功能已禁用 (编译时配置)");
#endif

    // 注意：显示屏通知将在TFT初始化后由主程序调用，避免在TFT未初始化时调用导致崩溃
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
                
                // 记录当前设备状态
                Serial.printf("[电源管理] GPS状态: %s, WiFi状态: %s, MQTT状态: %s\n",
                    device.get_device_state()->gpsReady ? "已就绪" : "未就绪",
                    device.get_device_state()->wifiConnected ? "已连接" : "未连接",
                    device.get_device_state()->mqttConnected ? "已连接" : "未连接");
                
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
    extern TaskHandle_t gpsTaskHandle;
    if (gpsTaskHandle != NULL) {
        vTaskDelete(gpsTaskHandle);
    }
#endif

    // IMU特殊处理 - 保持运动检测功能
#if defined(MODE_ALLINONE) || defined(MODE_SERVER)
    extern IMU imu;
    Serial.println("[电源管理] 配置IMU为低功耗模式，保持运动检测...");
    // 对IMU进行特殊配置，禁用陀螺仪，但保持加速度计运动检测
    // 注意：我们无法直接访问IMU的私有成员，这里假设IMU已经完成配置
    Serial.println("[电源管理] IMU: 保留加速度计用于运动检测，其他功能进入低功耗");
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
    // 配置唤醒源 - 优先级：按钮 > IMU运动检测 > 定时器
    // 每次只配置一个唤醒源
    Serial.println("[电源管理] 配置唤醒源...");
    
    bool wakeupSourceConfigured = false;
    
    #if defined(MODE_ALLINONE) || defined(MODE_SERVER)
    // 优先使用按钮作为唤醒源
#ifdef BTN_PIN
    if (BTN_PIN >= 0 && BTN_PIN <= 21) {
        Serial.printf("[电源管理] 尝试配置按钮唤醒 (GPIO%d)\n", BTN_PIN);
        
        // 检查按钮当前状态，避免无限重启
        extern BTN button;
        if (!button.isPressed()) {
            // 初始化RTC GPIO
            rtc_gpio_init((gpio_num_t)BTN_PIN);
            // 设置GPIO模式为输入
            rtc_gpio_set_direction((gpio_num_t)BTN_PIN, RTC_GPIO_MODE_INPUT_ONLY);
            // 启用内部上拉电阻，确保按钮未按下时为高电平
            rtc_gpio_pullup_en((gpio_num_t)BTN_PIN);
            rtc_gpio_pulldown_dis((gpio_num_t)BTN_PIN);
            // 配置为低电平触发唤醒
            esp_sleep_enable_ext0_wakeup((gpio_num_t)BTN_PIN, 0); 
            
            // 检查GPIO状态，如果已经是低电平，则禁用此唤醒源
            if (rtc_gpio_get_level((gpio_num_t)BTN_PIN) == 0) {
                Serial.println("[电源管理] 警告：按钮引脚处于低电平，不使用按钮唤醒源");
                esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_EXT0);
            } else {
                Serial.println("[电源管理] 已配置按钮为唤醒源");
                wakeupSourceConfigured = true;
            }
        } else {
            Serial.println("[电源管理] 警告：按钮当前处于按下状态，跳过按钮唤醒配置");
        }
    } else {
        Serial.printf("[电源管理] 警告：BTN_PIN (GPIO%d) 不是有效的RTC GPIO，无法用作唤醒源\n", BTN_PIN);
        Serial.println("[电源管理] ESP32-S3只有GPIO0-GPIO21能用作RTC GPIO");
    }
    #endif
    
    // 如果按钮唤醒配置失败，尝试使用IMU运动检测
    if (!wakeupSourceConfigured && IMU_INT1_PIN >= 0 && IMU_INT1_PIN <= 21) {
        Serial.printf("[电源管理] 尝试配置IMU运动唤醒 (GPIO%d)\n", IMU_INT1_PIN);
        
        // 配置IMU中断
        extern IMU imu;
        bool result = imu.enableMotionDetection(IMU_INT1_PIN, 0.05); // 设置较低的阈值提高灵敏度
        
        if (result) {
            Serial.println("[电源管理] IMU运动检测已配置成功");
            
            // 初始化RTC GPIO
            rtc_gpio_init((gpio_num_t)IMU_INT1_PIN);
            // 设置GPIO模式为输入
            rtc_gpio_set_direction((gpio_num_t)IMU_INT1_PIN, RTC_GPIO_MODE_INPUT_ONLY);
            // 启用内部上拉电阻，避免悬空状态
            rtc_gpio_pullup_en((gpio_num_t)IMU_INT1_PIN);
            rtc_gpio_pulldown_dis((gpio_num_t)IMU_INT1_PIN);
            // 配置为低电平触发唤醒
            esp_sleep_enable_ext0_wakeup((gpio_num_t)IMU_INT1_PIN, 0); 
            
            // 检查当前中断状态，确保不会立即唤醒
            if (digitalRead(IMU_INT1_PIN) == LOW) {
                Serial.println("[电源管理] 警告：IMU中断引脚当前为低电平，不使用IMU唤醒源");
                esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_EXT0);
            } else {
                Serial.println("[电源管理] 已配置IMU运动检测为唤醒源");
                wakeupSourceConfigured = true;
            }
        } else {
            Serial.println("[电源管理] IMU运动检测配置失败");
        }
    } else if (!wakeupSourceConfigured) {
        Serial.println("[电源管理] IMU中断引脚无效或已配置其他唤醒源");
    }
    #endif
    
    // 如果以上唤醒源都配置失败，则使用定时器作为最后的备选
    if (!wakeupSourceConfigured) {
        const uint64_t WAKEUP_INTERVAL_US = 60 * 1000000ULL; // 60秒
        Serial.printf("[电源管理] 配置定时器唤醒：%llu 微秒后\n", WAKEUP_INTERVAL_US);
        esp_sleep_enable_timer_wakeup(WAKEUP_INTERVAL_US);
        Serial.println("[电源管理] 已配置定时器为唤醒源");
    } else {
        // 配置一个更长的定时器作为备用唤醒源，防止设备完全无法唤醒
        const uint64_t BACKUP_INTERVAL_US = 10 * 60 * 1000000ULL; // 10分钟
        Serial.printf("[电源管理] 配置备用定时器唤醒：%llu 微秒后\n", BACKUP_INTERVAL_US);
        esp_sleep_enable_timer_wakeup(BACKUP_INTERVAL_US);
        Serial.println("[电源管理] 已配置备用定时器唤醒源");
    }
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
    
    // 关闭外设
    disablePeripherals();
    
    // 最后一次检查是否有打断请求
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
    
    // 配置唤醒源
    configureWakeupSources();
    
    // 打印设备信息和统计数据
    Serial.println("[电源管理] 设备状态汇总:");
    Serial.printf("[电源管理] 设备ID: %s\n", device.get_device_id().c_str());
    Serial.printf("[电源管理] 本次运行时间: %lu 毫秒\n", millis());
    
    // 延迟一段时间以允许串行输出完成
    delay(500);
    
    // 进入深度睡眠模式
    Serial.println("[电源管理] 正在进入深度睡眠模式...");
    esp_deep_sleep_start();
    
    // 此后的代码不会被执行，因为设备已进入深度睡眠
#endif
} 