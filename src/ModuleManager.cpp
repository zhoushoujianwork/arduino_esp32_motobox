#include "ModuleManager.h"

// 初始化静态实例指针
ModuleManager* ModuleManager::instance = nullptr;

// 全局实例
ModuleManager moduleManager;

// RTC变量保持（深度睡眠后保持）
RTC_DATA_ATTR int bootCount = 0;

ModuleManager::ModuleManager() : 
    gpsTaskHandle(nullptr),
    wifiInitTaskHandle(nullptr),
    lastGpsPublishTime(0),
    lastImuPublishTime(0),
    lastBlePublishTime(0),
    lastCompassPublishTime(0),
#if defined(MODE_ALLINONE) || defined(MODE_SERVER)
    gps(GPS_RX_PIN, GPS_TX_PIN),
    imu(IMU_SDA_PIN, IMU_SCL_PIN),
    button(BTN_PIN),
    mqtt(MQTT_SERVER, MQTT_PORT, MQTT_USER, MQTT_PASSWORD),
#endif
    bat(BAT_PIN, BAT_MIN_VOLTAGE, BAT_MAX_VOLTAGE),
    led(LED_PIN),
    powerManager()
{
    // 设置全局实例指针
    instance = this;
}

void ModuleManager::begin() {
    // 初始化串口
    Serial.begin(115200);
    delay(100);
    
    // 增加启动计数并打印
    bootCount++;
    Serial.println("[系统] 启动次数: " + String(bootCount));
    
    // 打印唤醒原因
    printWakeupReason();
    
    // 初始化硬件
    initializeHardware();
    
    // 处理从睡眠唤醒
    handleWakeup();

    Serial.println("[系统] 初始化完成");
}

void ModuleManager::createTasks() {
    // 创建任务
#if defined(MODE_ALLINONE) || defined(MODE_SERVER)
    xTaskCreate(taskWifi, "TaskWifi", 1024 * 10, NULL, 1, NULL);
    
    // 创建GPS任务
    xTaskCreatePinnedToCore(
        taskGps,
        "GPS Task",
        4096,
        NULL,
        1,
        &gpsTaskHandle,
        0
    );

#ifdef ENABLE_COMPASS
    // 创建罗盘任务
    xTaskCreatePinnedToCore(
        taskCompass,
        "Compass Task",
        2048,
        NULL,
        1,
        NULL,
        0
    );
#endif
#endif

    // 系统和数据处理任务在所有模式下都需要
    xTaskCreate(taskSystem, "TaskSystem", 1024 * 10, NULL, 2, NULL);
    xTaskCreate(taskDataProcessing, "TaskData", 1024 * 10, NULL, 1, NULL);
}

void ModuleManager::handleWakeup() {
#if defined(MODE_ALLINONE) || defined(MODE_SERVER)
    // 检查是否从深度睡眠唤醒
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    bool isWakeFromDeepSleep = (wakeup_reason != ESP_SLEEP_WAKEUP_UNDEFINED);
    
    // 显示屏初始化完成后，再初始化 GPS
    Serial.println("[系统] 初始化GPS...");
    
    if (isWakeFromDeepSleep) {
        // 如果是从睡眠中唤醒，需要先唤醒GPS模块
        Serial.println("[系统] 从睡眠唤醒，正在唤醒GPS模块...");
        gps.wakeup();
    }
#endif
}

void ModuleManager::printWakeupReason() {
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

    switch(wakeup_reason) {
        case ESP_SLEEP_WAKEUP_EXT0: 
            Serial.println("[系统] 从外部RTC_IO唤醒 (运动检测)");
            // 如果是通过IMU中断唤醒，则重新启用IMU
#ifdef IMU_INT_PIN
            if (gpio_get_level((gpio_num_t)IMU_INT_PIN) == 1) {
                Serial.println("[系统] 检测到IMU运动中断引脚处于高电平，确认为运动检测唤醒");
            }
#endif
            break;
        case ESP_SLEEP_WAKEUP_EXT1: 
            {
#if defined(BTN_PIN)
                uint64_t wakeup_pin_mask = esp_sleep_get_ext1_wakeup_status();
                if (wakeup_pin_mask & (1ULL << BTN_PIN)) {
                    Serial.println("[系统] 从按钮唤醒");
                } else {
                    Serial.println("[系统] 从外部RTC_CNTL唤醒");
                }
#endif
            }
            break;
        case ESP_SLEEP_WAKEUP_TIMER: 
            Serial.println("[系统] 从定时器唤醒");
            break;
        case ESP_SLEEP_WAKEUP_TOUCHPAD: 
            Serial.println("[系统] 从触摸唤醒");
            break;
        case ESP_SLEEP_WAKEUP_ULP: 
            Serial.println("[系统] 从ULP唤醒");
            break;
        default: 
            Serial.printf("[系统] 从非深度睡眠唤醒，原因代码: %d\n", wakeup_reason);
            break;
    }
    
    // 打印系统信息
    Serial.printf("[系统] ESP32-S3芯片ID: %llX\n", ESP.getEfuseMac());
    Serial.printf("[系统] 总运行内存: %d KB\n", ESP.getHeapSize() / 1024);
    Serial.printf("[系统] 可用运行内存: %d KB\n", ESP.getFreeHeap() / 1024);
    Serial.printf("[系统] CPU频率: %d MHz\n", ESP.getCpuFreqMHz());
}

void ModuleManager::initializeHardware() {
    // 检查是否从深度睡眠唤醒
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    bool isWakeFromDeepSleep = (wakeup_reason != ESP_SLEEP_WAKEUP_UNDEFINED);
    
    // 打印启动信息
    if (isWakeFromDeepSleep) {
        Serial.println("[系统] 从深度睡眠唤醒，重新初始化系统...");
    } else {
        Serial.printf("[系统] 系统正常启动，版本: %s\n", VERSION);
    }
    
    // LED初始化
    led.begin();
    led.setMode(LED::OFF);

    // 初始化设备
    device.init();

#if defined(MODE_ALLINONE) || defined(MODE_SERVER)
    // IMU初始化
    imu.begin();
    
    // 如果是从浅睡眠中唤醒并且是因为运动检测，则需要清除IMU中断状态
    if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT0) {
        Serial.println("[系统] 从IMU运动检测唤醒，清除中断状态");
        // 调用disableMotionDetection来清除中断状态
        imu.disableMotionDetection();
    }
    
#ifdef ENABLE_COMPASS
    // 初始化罗盘
    compass.begin();
#endif
    
    // 初始化GPS
    gps.begin();
    
    // 在单独任务中启动WiFi初始化（将会并行进行）
    xTaskCreate(taskWifiInit, "TaskWifiInit", 1024 * 10, NULL, 1, &wifiInitTaskHandle);
#endif

    // 电源管理初始化
    powerManager.begin();

#if defined(MODE_ALLINONE) || defined(MODE_CLIENT)
    // 如果从深度睡眠唤醒，设置标志位跳过动画
    if (isWakeFromDeepSleep) {
        tft_is_waking_from_sleep = true;
        Serial.println("[系统] 设置显示屏唤醒标志");
    }
    
    // 显示屏初始化
    Serial.println("[系统] 初始化显示屏...");
    tft_begin();
#endif

#ifdef MODE_SERVER
    // 蓝牙服务器初始化
    bs.setup();
#endif

#ifdef MODE_CLIENT
    // 蓝牙客户端初始化
    bc.setup();
#endif
}

void ModuleManager::handleButtonEvents() {
#if defined(MODE_ALLINONE) || defined(MODE_SERVER)
    // 检查是否处于低功耗状态
    if (powerManager.getPowerState() != POWER_STATE_NORMAL) {
        if (button.isPressed()) {
            Serial.println("[按钮] 检测到按钮按下，打断低功耗模式");
            powerManager.interruptLowPowerMode();
            
            // 等待按钮释放，避免重复触发
            while (button.isPressed()) {
                delay(10);
                button.loop();
            }
        }
    } 
    // 正常模式下的按钮处理
    else {
        // 点击事件 - 切换GPS刷新率
        if (button.isClicked()) {
            Serial.println("[按钮] 检测到点击，切换GPS刷新率");
            
            // 依次切换可用的频率: 1Hz -> 2Hz -> 5Hz -> 10Hz -> 1Hz...
            static int hzValues[] = {1, 2, 5, 10};
            static int hzIndex = 1; // 默认从2Hz开始 (index=1)
            
            hzIndex = (hzIndex + 1) % 4; // 循环切换
            int newHz = hzValues[hzIndex];
            
            vTaskSuspend(gpsTaskHandle);
            delay(100);
            if (gps.setGpsHz(newHz)) {
                Serial.printf("[GPS] 手动设置GPS更新率为%dHz\n", newHz);
            } else {
                Serial.println("[GPS] 设置GPS更新率失败");
            }
            // 恢复GPS任务
            vTaskResume(gpsTaskHandle);
        }

        // 长按事件 - 重置WiFi
        static bool longPressHandled = false;  // 添加静态变量记录长按是否已处理
        bool currentlyLongPressed = button.isLongPressed();
        
        if (currentlyLongPressed && !longPressHandled) {  // 只有未处理过的长按才执行
            Serial.println("[按钮] 检测到长按，重置WIFI");
            
            if (!wifiManager.getConfigMode()) {
                wifiManager.reset();
            }
            
            longPressHandled = true;  // 标记为已处理
        } else if (!currentlyLongPressed) {
            // 不是长按状态时重置标志，以便下次长按可以触发
            longPressHandled = false;
        }
    }
#endif
}

// 任务函数实现 - 静态版本

void ModuleManager::taskWifiInit(void *parameter) {
    if (instance) {
        instance->wifiInitImpl();
    }
    // 任务完成后自动删除
    vTaskDelete(NULL);
}

void ModuleManager::taskWifi(void *parameter) {
    if (instance) {
        while (true) {
            instance->wifiLoopImpl();
            delay(5);
        }
    }
}

void ModuleManager::taskGps(void *parameter) {
    if (instance) {
        while (true) {
            instance->gpsLoopImpl();
            delay(5);
        }
    }
}

#ifdef ENABLE_COMPASS
void ModuleManager::taskCompass(void *parameter) {
    if (instance) {
        while (true) {
            instance->compassLoopImpl();
            delay(5);
        }
    }
}
#endif

void ModuleManager::taskSystem(void *parameter) {
    if (instance) {
        while (true) {
            instance->systemLoopImpl();
            delay(5);
        }
    }
}

void ModuleManager::taskDataProcessing(void *parameter) {
    if (instance) {
        while (true) {
            instance->dataProcessingImpl();
            delay(5);
        }
    }
}

// 任务实现 - 实例方法

void ModuleManager::wifiInitImpl() {
#if defined(MODE_ALLINONE) || defined(MODE_SERVER)
    Serial.println("[系统] 初始化WiFi连接...");
    wifiManager.begin();
#endif
}

void ModuleManager::wifiLoopImpl() {
#if defined(MODE_ALLINONE) || defined(MODE_SERVER)
    wifiManager.loop();
#endif
}

void ModuleManager::gpsLoopImpl() {
#if defined(MODE_ALLINONE) || defined(MODE_SERVER)
    // 使用loop函数解析NMEA数据并更新设备GPS信息
    gps.loop();
    
    // 打印GPS统计信息，显示接收质量
    gps.printGpsStats();
#endif
}

#ifdef ENABLE_COMPASS
void ModuleManager::compassLoopImpl() {
    // 处理罗盘数据
    compass.loop();
}
#endif

void ModuleManager::systemLoopImpl() {
    // GPS更新率自动调整的时间记录
    static unsigned long lastGpsRateAdjustTime = 0;
 
    // LED状态更新
    led.loop();
    
    // 电池监控
#ifdef MODE_CLIENT
    if (!device.get_device_state()->bleConnected) {
        bat.loop();
    }
#else
    bat.loop();
#endif

#if defined(MODE_ALLINONE) || defined(MODE_SERVER)
    // LED状态设置 - 连接状态指示
    bool isConnected = device.get_device_state()->mqttConnected && 
                      device.get_device_state()->wifiConnected;
    led.setMode(isConnected ? LED::BLINK_DUAL : LED::OFF);

    // 按钮状态更新
    button.loop();
    
    // 按钮处理逻辑
    handleButtonEvents();
    
    // 每5秒自动调整GPS更新率
    if (millis() - lastGpsRateAdjustTime >= 5000) {
        // 暂停GPS任务以避免数据读取冲突
        vTaskSuspend(gpsTaskHandle);
        
        // 调整GPS更新率
        gps.autoAdjustUpdateRate();
        
        // 恢复GPS任务
        vTaskResume(gpsTaskHandle);
        
        // 更新时间戳
        lastGpsRateAdjustTime = millis();
    }
#endif

    // 电源管理 - 始终保持处理
    powerManager.loop();
}

void ModuleManager::dataProcessingImpl() {
#if defined(MODE_ALLINONE) || defined(MODE_SERVER)
    // IMU数据处理
    imu.loop();
    
    // MQTT数据发布
    mqtt.loop();
    
    // GPS数据发布 (1Hz)
    if (millis() - lastGpsPublishTime >= GPS_PUBLISH_INTERVAL) {
        mqtt.publishGPS(*device.get_gps_data());
        lastGpsPublishTime = millis();
        
        // 更新GPS就绪状态
        device.set_gps_ready(device.get_gps_data()->satellites > 3);
    }
    
    // IMU数据发布 (2Hz)
    if (millis() - lastImuPublishTime >= IMU_PUBLISH_INTERVAL) {
        mqtt.publishIMU(*device.get_imu_data());
        lastImuPublishTime = millis();
    }

#ifdef ENABLE_COMPASS
    // 如果罗盘已就绪，发布方位角数据
    if (device.get_device_state()->compassReady) {
        if (millis() - lastCompassPublishTime > MQTT_COMPASS_PUBLISH_INTERVAL) {
            // 创建包含方位角的JSON数据
            StaticJsonDocument<128> doc;
            doc["heading"] = device.get_gps_data()->heading;
            String compassJson;
            serializeJson(doc, compassJson);
            
            // 发布到MQTT
            mqtt.publish(MQTT_TOPIC_COMPASS, compassJson);
            lastCompassPublishTime = millis();
        }
    }
#endif
#endif

#ifdef MODE_CLIENT
    // 蓝牙客户端处理
    bc.loop();
#endif

#if defined(MODE_ALLINONE) || defined(MODE_CLIENT)
    // 显示屏更新
    tft_loop();
#endif

#ifdef MODE_SERVER
    // 蓝牙服务器广播 (1Hz)
    if (millis() - lastBlePublishTime >= BLE_PUBLISH_INTERVAL) {
        bs.loop();
        lastBlePublishTime = millis();
    }
#endif
} 