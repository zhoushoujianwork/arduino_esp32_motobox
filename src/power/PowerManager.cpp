#include "PowerManager.h"
#include "btn/BTN.h"
#include "esp_wifi.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "driver/periph_ctrl.h"
#include "soc/periph_defs.h"
#include "led/PWMLED.h"
#include "device.h"
#include "utils/PreferencesUtils.h"
#include "mqtt/MQTT.h"

// 初始化静态变量
#if ENABLE_SLEEP
RTC_DATA_ATTR bool PowerManager::sleepEnabled = true; // 默认启用休眠功能
#else
RTC_DATA_ATTR bool PowerManager::sleepEnabled = false; // 默认禁用休眠功能
#endif

extern MQTT mqtt;

PowerManager powerManager;


PowerManager::PowerManager()
{
    // 设置默认值
    lastMotionTime = 0;
    powerState = POWER_STATE_NORMAL;
    sleepTimeSec = device.get_device_state()->sleep_time;
}

void PowerManager::begin()
{
    powerState = POWER_STATE_NORMAL;

    // 启动时从存储读取休眠时间（秒），如无则用默认值
    sleepTimeSec = PreferencesUtils::loadULong(PreferencesUtils::NS_POWER, PreferencesUtils::KEY_SLEEP_TIME, device.get_device_state()->sleep_time);
    idleThreshold = sleepTimeSec * 1000;

    // 处理唤醒事件
    handleWakeup();

    Serial.println("[电源管理] 休眠功能已启用 (编译时配置)");
    Serial.printf("[电源管理] 当前休眠时间: %lu 秒\n", sleepTimeSec);
}

void PowerManager::handleWakeup()
{
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

    switch (wakeup_reason)
    {
    case ESP_SLEEP_WAKEUP_EXT0:
    {
        if (IMU_INT_PIN >= 0 && IMU_INT_PIN <= 21)
        {
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
            if (imu.checkWakeOnMotionEvent())
            {
                Serial.println("[电源管理] 确认为WakeOnMotion事件唤醒");
            }
            else
            {
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
        if (wakeup_reason != ESP_SLEEP_WAKEUP_UNDEFINED)
        {
            Serial.printf("[电源管理] 其他唤醒原因: %d\n", wakeup_reason);
        }
        break;
    }
}

void PowerManager::configurePowerDomains()
{
    // 配置ESP32-S3特定的省电选项
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);    // 保持RTC外设供电
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_ON);  // 保持RTC慢速内存
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF); // 关闭RTC快速内存
    esp_sleep_pd_config(ESP_PD_DOMAIN_XTAL, ESP_PD_OPTION_OFF);         // 关闭XTAL
}

bool PowerManager::configureWakeupSources()
{
    Serial.println("[电源管理] 🔧 开始配置唤醒源...");

    // 配置IMU运动唤醒
    if (IMU_INT_PIN >= 0 && IMU_INT_PIN <= 21)
    { // 确保是有效的RTC GPIO
        Serial.printf("[电源管理] 配置IMU唤醒引脚 GPIO%d...\n", IMU_INT_PIN);

        // 初始化RTC GPIO
        esp_err_t ret = rtc_gpio_init((gpio_num_t)IMU_INT_PIN);
        if (ret != ESP_OK)
        {
            Serial.printf("[电源管理] ❌ RTC GPIO初始化失败: %s\n", esp_err_to_name(ret));
            return false;
        }
        Serial.println("[电源管理] ✅ RTC GPIO初始化成功");

        rtc_gpio_set_direction((gpio_num_t)IMU_INT_PIN, RTC_GPIO_MODE_INPUT_ONLY);
        rtc_gpio_pullup_en((gpio_num_t)IMU_INT_PIN);    // 使用上拉电阻
        rtc_gpio_pulldown_dis((gpio_num_t)IMU_INT_PIN); // 禁用下拉
        Serial.println("[电源管理] ✅ RTC GPIO方向和上拉配置完成");

        // WakeOnMotion模式中，官方例子建议检查默认引脚值来确定触发方式
        // 由于我们在configWakeOnMotion中设置defaultPinValue=1，所以检测低电平触发
        ret = esp_sleep_enable_ext0_wakeup((gpio_num_t)IMU_INT_PIN, 0); // 低电平触发
        if (ret != ESP_OK)
        {
            Serial.printf("[电源管理] ❌ EXT0唤醒配置失败: %s\n", esp_err_to_name(ret));
            return false;
        }
        Serial.printf("[电源管理] ✅ EXT0唤醒配置成功 (GPIO%d, 低电平触发)\n", IMU_INT_PIN);

        // 配置IMU的运动检测中断
        extern IMU imu;
        Serial.println("[电源管理] 配置IMU为深度睡眠模式...");
        bool imuConfigured = imu.configureForDeepSleep(); // 使用新的WakeOnMotion配置
        if (!imuConfigured)
        {
            Serial.println("[电源管理] ❌ IMU深度睡眠配置失败");
            return false;
        }
        Serial.println("[电源管理] ✅ IMU深度睡眠配置成功");

        // 添加延迟确保IMU配置稳定
        delay(100);

        Serial.printf("[电源管理] IMU WakeOnMotion唤醒源配置完成 (GPIO%d, 低电平触发)\n", IMU_INT_PIN);
    }
    else
    {
        Serial.printf("[电源管理] ❌ 无效的IMU中断引脚: %d\n", IMU_INT_PIN);
        return false;
    }

    // 配置定时器唤醒作为备份
    Serial.println("[电源管理] 配置定时器唤醒...");
    const uint64_t WAKEUP_INTERVAL_US = 30 * 60 * 1000000ULL; // 30分钟
    esp_err_t ret = esp_sleep_enable_timer_wakeup(WAKEUP_INTERVAL_US);
    if (ret != ESP_OK)
    {
        Serial.printf("[电源管理] ❌ 定时器唤醒配置失败: %s\n", esp_err_to_name(ret));
        return false;
    }
    Serial.println("[电源管理] ✅ 定时器唤醒源配置完成");
    return true; // 全部配置成功
}

void PowerManager::disablePeripherals()
{
    Serial.println("[电源管理] 正在关闭所有外设电路...");

    // ===== 0. 断开MQTT连接 =====
    mqtt.disconnect();

    // ===== 1. 通信模块关闭 =====

    // WiFi完全关闭
    wifiManager.safeDisableWiFi();

    // 蓝牙完全关闭
    btStop();
    // 添加安全检查，避免重复关闭蓝牙控制器
    // esp_bt_controller_disable();
    // esp_bt_controller_deinit();
    Serial.println("[电源管理] 蓝牙已安全关闭");

// ===== 2. 外设任务停止 =====

// GPS任务停止
#if defined(MODE_ALLINONE) || defined(MODE_SERVER)
    extern TaskHandle_t gpsTaskHandle;
    if (gpsTaskHandle != NULL)
    {
        vTaskDelete(gpsTaskHandle);
        gpsTaskHandle = NULL;
        Serial.println("[电源管理] GPS任务已停止");
    }

// GPS串口关闭
#ifdef GPS_RX_PIN
    Serial2.end(); // 假设GPS使用Serial2
    pinMode(GPS_RX_PIN, INPUT);
    pinMode(GPS_TX_PIN, INPUT);
    Serial.println("[电源管理] GPS串口已关闭");
#endif
#endif

    // ===== 3. 显示屏完全关闭 =====

#ifdef MODE_ALLINONE
    extern void tft_sleep();
    tft_sleep();

// 安全关闭SPI总线
// SPI.end();  // 暂时注释掉，可能引起问题

// 关闭显示屏相关GPIO
#ifdef TFT_CS
    pinMode(TFT_CS, INPUT);
#endif
#ifdef TFT_DC
    pinMode(TFT_DC, INPUT);
#endif
#ifdef TFT_RST
    pinMode(TFT_RST, INPUT);
#endif
#ifdef TFT_BL
    pinMode(TFT_BL, INPUT);
    digitalWrite(TFT_BL, LOW); // 关闭背光
#endif

    Serial.println("[电源管理] 显示屏已完全关闭");
#endif

    // ===== 4. LED控制关闭 =====

#ifdef PWM_LED_PIN
    extern PWMLED pwmLed;
    pwmLed.setMode(PWMLED::OFF);
    FastLED.show();
    delay(20); // 确保数据已发出
    FastLED.show();
    delay(20); // 确保数据已发出
    FastLED.show();
    digitalWrite(PWM_LED_PIN, LOW); // 强制拉低
    delay(5);
    pinMode(PWM_LED_PIN, INPUT); // 再切换为高阻
    Serial.println("[电源管理] PWM LED已关闭");
#endif

#ifdef LED_PIN
    digitalWrite(LED_PIN, LOW);
    pinMode(LED_PIN, INPUT);
    Serial.println("[电源管理] 普通LED已关闭");
#endif

// ===== 5. I2C总线关闭 =====

// 关闭IMU I2C（但保持IMU的WakeOnMotion功能）
#if defined(MODE_ALLINONE) || defined(MODE_SERVER)
    extern IMU imu;
    // imu.configureForDeepSleep(); // 使用WakeOnMotion配置（已移至configureWakeupSources，避免重复配置导致超时）

// 保守地关闭其他I2C设备
// Wire.end();    // 暂时注释，可能影响IMU
// Wire1.end();   // 暂时注释，可能影响IMU

// 将非关键I2C引脚设置为输入模式
#ifdef GPS_COMPASS_SDA
    pinMode(GPS_COMPASS_SDA, INPUT);
#endif
#ifdef GPS_COMPASS_SCL
    pinMode(GPS_COMPASS_SCL, INPUT);
#endif

    Serial.println("[电源管理] I2C设备已配置为低功耗模式");
#endif

    // ===== 6. 罗盘传感器关闭 =====

#ifdef MODE_ALLINONE
#ifdef GPS_COMPASS_SDA
    pinMode(GPS_COMPASS_SDA, INPUT);
#endif
#ifdef GPS_COMPASS_SCL
    pinMode(GPS_COMPASS_SCL, INPUT);
#endif
    Serial.println("[电源管理] 罗盘传感器已关闭");
#endif

    // ===== 7. ADC和电池监测关闭 =====

    // 安全关闭ADC
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_11);
    // 移除adc_power_release()调用，因为可能导致崩溃
    // 改为使用adc_power_off()或让系统自动管理ADC电源

#ifdef BAT_PIN
    pinMode(BAT_PIN, INPUT);
#endif

    Serial.println("[电源管理] ADC已配置为低功耗模式");

    // ===== 8. 按钮引脚配置 =====

#ifdef BTN_PIN
    // 保持按钮引脚的上拉配置，作为备用唤醒源
    pinMode(BTN_PIN, INPUT_PULLUP);
    Serial.println("[电源管理] 按钮引脚保持上拉配置");
#endif

    // ===== 9. 未使用GPIO设置为输入模式 =====

    // 跳过GPIO循环设置，避免引脚冲突（引脚使用随时会变）
    Serial.println("[电源管理] 跳过GPIO批量设置（避免引脚冲突）");

    // ===== 10. 时钟和外设时钟关闭 =====

    // 关闭不必要的时钟
    periph_module_disable(PERIPH_LEDC_MODULE);
    periph_module_disable(PERIPH_I2S0_MODULE);
    periph_module_disable(PERIPH_I2S1_MODULE);
    periph_module_disable(PERIPH_UART1_MODULE);
    periph_module_disable(PERIPH_UART2_MODULE);

    Serial.println("[电源管理] 外设时钟已关闭");

    // ===== 11. CPU频率调整 =====

    // 降低CPU频率到最低
    setCpuFrequencyMhz(10); // 10MHz最低频率
    Serial.println("[电源管理] CPU频率已降至10MHz");

    Serial.println("[电源管理] ✅ 所有外设电路已完全关闭");

#ifdef GPS_WAKE_PIN
    pinMode(GPS_WAKE_PIN, OUTPUT);
    digitalWrite(GPS_WAKE_PIN, LOW);
    rtc_gpio_hold_en((gpio_num_t)GPS_WAKE_PIN); // 保持低电平
    
    Serial.println("[电源管理] GPS_WAKE_PIN 已拉低并保持，GPS进入休眠");
#endif
}

void PowerManager::loop()
{
    static unsigned long lastMotionCheck = 0;
    unsigned long now = millis();

    // 只每隔200ms检测一次运动
    if (now - lastMotionCheck >= 200) {
        lastMotionCheck = now;


#if !ENABLE_SLEEP
        // 如果编译时禁用了休眠功能，始终保持在正常状态
        if (powerState != POWER_STATE_NORMAL)
        {
            powerState = POWER_STATE_NORMAL;
        }
        return;
#else
        // 编译时启用了休眠功能，但需要检查运行时状态
        if (!sleepEnabled)
        {
            // 保持在正常状态，不进入休眠
            if (powerState != POWER_STATE_NORMAL)
            {
                powerState = POWER_STATE_NORMAL;
            }
            return;
        }

        // 仅在正常工作状态下检测运动和空闲状态
        if (powerState == POWER_STATE_NORMAL)
        {
            // 检测设备是否有运动
            if (imu.detectMotion())
            {
                // 如果有运动，更新最后一次运动时间
                lastMotionTime = millis();
            }
            else
            {
                // 检查设备是否空闲足够长的时间
                if (isDeviceIdle())
                {
                    Serial.printf("[电源管理] 设备已静止超过%lu秒，准备进入低功耗模式...\n", sleepTimeSec);
                    Serial.printf("[电源管理] 电池状态: %d%%, 电压: %dmV\n",
                                  device.get_device_state()->battery_percentage,
                                  device.get_device_state()->battery_voltage);
                    
                    enterLowPowerMode();
                }
            }
        }
#endif
    }
    // 其他低频逻辑可以继续执行
}

bool PowerManager::isDeviceIdle()
{
    // 新增：AP模式下有客户端连接时不判定为空闲
    if (wifiManager.isAPModeActive() && wifiManager.hasAPClient()) {
        // Serial.println("[电源管理] AP模式下有客户端连接，禁止休眠");
        return false;
    }
    // 检查设备是否空闲足够长的时间
    return (millis() - lastMotionTime) > idleThreshold;
}

void PowerManager::enterLowPowerMode()
{
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
    if (!sleepEnabled)
    {
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

    for (int i = countdownTime; i > 0; i--)
    {
        Serial.printf("[电源管理] 倒计时: %d 秒\n", i);

// 设置屏幕亮度，逐渐降低
#ifdef MODE_ALLINONE
        int brightness = map(i, countdownTime, 1, maxBrightness, maxBrightness / 10);
        tft_set_brightness(brightness);
        Serial.printf("[电源管理] 屏幕亮度设置为 %d/%d\n", brightness, maxBrightness);
#endif

        // 使用更小的时间间隔检查中断请求，提高响应速度
        unsigned long countdownStartTime = millis();
        while (millis() - countdownStartTime < 1000)
        { // 每秒分为多个小间隔检查
            // 检查是否已请求打断
            if (imu.detectMotion())
            {
                Serial.println("[电源管理] 收到打断请求，取消进入低功耗模式");
                interruptLowPowerMode();
                // 恢复屏幕亮度到最大
#ifdef MODE_ALLINONE
        tft_set_brightness(maxBrightness);
        Serial.println("[电源管理] 恢复屏幕亮度到最大");
#endif

// 恢复屏幕亮度到最大
#ifdef MODE_ALLINONE
                tft_set_brightness(maxBrightness);
                Serial.println("[电源管理] 恢复屏幕亮度到最大");
#endif
                return; // 退出函数，不进入低功耗模式
            }

            delay(10); // 小间隔检查，更快响应

        }
    }

    // 设置电源状态为准备睡眠
    powerState = POWER_STATE_PREPARING_SLEEP;

    Serial.println("[电源管理] 倒计时结束，开始关闭外设...");

    // 在进入深度睡眠前保存关键状态到RTC内存
    RTC_DATA_ATTR static uint32_t sleepCount = 0;
    sleepCount++;

    Serial.println("[电源管理] ⏸️  开始执行外设关闭流程...");
    // 优化外设关闭顺序
    disablePeripherals();
    Serial.println("[电源管理] ✅ 外设关闭完成");

    Serial.println("[电源管理] ⏸️  开始配置电源域...");
    // 配置电源域
    configurePowerDomains();
    Serial.println("[电源管理] ✅ 电源域配置完成");

    Serial.println("[电源管理] ⏸️  开始配置唤醒源...");
    // 配置唤醒源
    if (!configureWakeupSources()) {
        Serial.println("[电源管理] ❌ 唤醒源配置失败，终止休眠流程，恢复正常模式");
        powerState = POWER_STATE_NORMAL;
#ifdef MODE_ALLINONE
        tft_set_brightness(255);
        Serial.println("[电源管理] 恢复屏幕亮度到最大");
#endif
        return;
    }
    Serial.println("[电源管理] ✅ 唤醒源配置完成");

    // 打印设备信息和统计数据
    Serial.println("[电源管理] 设备状态汇总:");
    Serial.printf("[电源管理] 设备ID: %s\n", device.get_device_id().c_str());
    Serial.printf("[电源管理] 本次运行时间: %lu 毫秒\n", millis());
    Serial.printf("[电源管理] 累计睡眠次数: %d\n", sleepCount);

    // 延迟一段时间以允许串口输出完成
    Serial.flush();
    delay(100);

    // 进入深度睡眠模式
    Serial.println("[电源管理] 🌙 正在进入深度睡眠模式...");
    Serial.flush(); // 确保最后的日志输出
    delay(50);      // 给串口更多时间

    esp_deep_sleep_start();

    // 这行代码永远不应该被执行到
    Serial.println("[电源管理] ❌ 错误：深度睡眠启动失败！");
#endif
}

void PowerManager::interruptLowPowerMode()
{
    // 重置电源状态
    powerState = POWER_STATE_NORMAL;
    // 重置运动检测窗口时间
    lastMotionTime = millis();
}

/**
 * 打印唤醒原因
 */
void PowerManager::printWakeupReason()
{
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    switch (wakeup_reason)
    {
    case ESP_SLEEP_WAKEUP_EXT0:
        if (IMU_INT_PIN >= 0 && IMU_INT_PIN <= 21)
        {
            Serial.printf("[系统] 从IMU运动检测唤醒 (GPIO%d)\n", IMU_INT_PIN);
        }
        else
        {
            Serial.println("[系统] 从外部RTC_IO唤醒，但IMU引脚配置无效");
        }
        break;
    case ESP_SLEEP_WAKEUP_EXT1:
        Serial.println("[系统] 从外部RTC_CNTL唤醒");
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
    Serial.printf("[系统] ESP32-S3芯片ID: %llX\n", ESP.getEfuseMac());
    Serial.printf("[系统] 总运行内存: %d KB\n", ESP.getHeapSize() / 1024);
    Serial.printf("[系统] 可用运行内存: %d KB\n", ESP.getFreeHeap() / 1024);
    Serial.printf("[系统] CPU频率: %d MHz\n", ESP.getCpuFreqMHz());
}

/**
 * 检查唤醒原因并处理
 */
void PowerManager::checkWakeupCause()
{
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    switch (wakeup_reason)
    {
    case ESP_SLEEP_WAKEUP_EXT0:
        Serial.println("[系统] 通过外部中断唤醒 (EXT0)");
        if (IMU_INT_PIN >= 0 && IMU_INT_PIN <= 21)
        {
            if (digitalRead(IMU_INT_PIN) == LOW)
            {
                Serial.println("[系统] 检测到IMU运动唤醒");
                // 这里可以添加特定的运动唤醒处理逻辑
            }
            else
            {
                Serial.println("[系统] 检测到按钮唤醒");
                // 这里可以添加特定的按钮唤醒处理逻辑
            }
        }
        break;
    case ESP_SLEEP_WAKEUP_TIMER:
        Serial.println("[系统] 通过定时器唤醒");
        break;
    default:
        Serial.printf("[系统] 唤醒原因: %d\n", wakeup_reason);
        break;
    }
}

// 新增：设置休眠时间（秒），并保存到存储，同时更新设备状态，重新计时休眠
void PowerManager::setSleepTime(unsigned long seconds) {
    device.get_device_state()->sleep_time = seconds; // 更新设备状态
    sleepTimeSec = seconds;
    idleThreshold = sleepTimeSec * 1000;
    lastMotionTime = millis(); // 新增：重置空闲计时
    PreferencesUtils::saveULong(PreferencesUtils::NS_POWER, PreferencesUtils::KEY_SLEEP_TIME, sleepTimeSec);
    Serial.printf("[电源管理] 休眠时间已更新并保存: %lu 秒\n", sleepTimeSec);
}

// 新增：获取休眠时间（秒）
unsigned long PowerManager::getSleepTime() const {
    if (device.get_device_state()->sleep_time != sleepTimeSec) {
        Serial.printf("[电源管理] 出现问题：休眠时间不一致 %lu 秒，device: %lu 秒\n", sleepTimeSec, device.get_device_state()->sleep_time);
    }
    return sleepTimeSec;
}
