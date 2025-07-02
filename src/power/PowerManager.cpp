#include "PowerManager.h"
#include "btn/BTN.h"
#include "esp_wifi.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "driver/periph_ctrl.h"
#include "soc/periph_defs.h"
#include "device.h"

// 初始化静态变量
#ifdef ENABLE_SLEEP
RTC_DATA_ATTR bool PowerManager::sleepEnabled = true;
#else
RTC_DATA_ATTR bool PowerManager::sleepEnabled = false;
#endif

PowerManager powerManager;

PowerManager::PowerManager()
{
    Serial.println("[电源管理] 初始化开始");
    // 设置默认值
    lastMotionTime = 0;
    powerState = POWER_STATE_NORMAL;
    sleepTimeSec = get_device_state()->sleep_time;

    powerState = POWER_STATE_NORMAL;
}

void PowerManager::begin()
{
    Serial.println("[电源管理] 初始化开始");

    // 启动时从存储读取休眠时间（秒），如无则用默认值
    sleepTimeSec = PreferencesUtils::loadSleepTime();
    idleThreshold = sleepTimeSec * 1000;

    // 处理唤醒事件
    handleWakeup();

    Serial.println("[电源管理] 休眠功能已启用 (编译时配置)");
    Serial.printf("[电源管理] 当前休眠时间: %lu 秒\n", sleepTimeSec);
}

void PowerManager::handleWakeup()
{
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    Serial.printf("[电源管理] 唤醒原因: %d\n", wakeup_reason);

    // 解除所有GPIO保持
    gpio_deep_sleep_hold_dis();

    switch (wakeup_reason)
    {
    case ESP_SLEEP_WAKEUP_EXT0:
    {
#if defined(ENABLE_IMU) && defined(IMU_INT_PIN)
        if (IMU_INT_PIN >= 0 && IMU_INT_PIN <= 21)
        {
            // 等待引脚状态稳定
            delay(100);
            int pinState = digitalRead(IMU_INT_PIN);
            Serial.printf("[电源管理] IMU引脚状态: %d\n", pinState);

            // 恢复IMU
            extern IMU imu;
            Serial.println("[电源管理] 从IMU运动唤醒，恢复IMU状态");
            imu.restoreFromDeepSleep();

            if (imu.checkWakeOnMotionEvent())
            {
                Serial.println("[电源管理] ✅ 确认为运动唤醒事件");
            }
            else
            {
                Serial.println("[电源管理] ⚠️ 未检测到运动事件，可能为其他原因唤醒");
            }
        }
#endif
        break;
    }
    case ESP_SLEEP_WAKEUP_TIMER:
        Serial.println("[电源管理] ⏰ 定时器唤醒");
        break;
    default:
        Serial.println("[电源管理] 🔄 首次启动或重置");
        break;
    }

    // 重置运动检测时间
    lastMotionTime = millis();
    
    // 恢复SD卡
#ifdef ENABLE_SDCARD
    extern SDManager sdManager;
    if (!sdManager.isInitialized()) {
        Serial.println("[电源管理] 从睡眠恢复，重新初始化SD卡");
        sdManager.restoreFromSleep();
        if (sdManager.isInitialized()) {
            // 记录唤醒日志
            String wakeupLog = "设备从深度睡眠唤醒，原因: ";
            switch (wakeup_reason) {
                case ESP_SLEEP_WAKEUP_EXT0:
                    wakeupLog += "运动检测";
                    break;
                case ESP_SLEEP_WAKEUP_TIMER:
                    wakeupLog += "定时器";
                    break;
                default:
                    wakeupLog += "其他";
                    break;
            }
            sdManager.writeLog(wakeupLog);
        }
    }
#endif
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

    // 1. 先禁用所有唤醒源
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);

// 2. 配置IMU运动唤醒
#if defined(ENABLE_IMU) && defined(IMU_INT_PIN)
    if (IMU_INT_PIN >= 0 && IMU_INT_PIN <= 21)
    {
        // 检查是否为有效的RTC GPIO
        if (!rtc_gpio_is_valid_gpio((gpio_num_t)IMU_INT_PIN))
        {
            Serial.printf("[电源管理] ❌ GPIO%d 不是有效的RTC GPIO\n", IMU_INT_PIN);
            return false;
        }

        // 解除GPIO保持状态
        rtc_gpio_hold_dis((gpio_num_t)IMU_INT_PIN);

        // 初始化RTC GPIO
        esp_err_t ret = rtc_gpio_init((gpio_num_t)IMU_INT_PIN);
        if (ret != ESP_OK)
        {
            Serial.printf("[电源管理] ❌ RTC GPIO初始化失败: %s\n", esp_err_to_name(ret));
            return false;
        }

        // 配置GPIO方向
        rtc_gpio_set_direction((gpio_num_t)IMU_INT_PIN, RTC_GPIO_MODE_INPUT_ONLY);
        
        // GPIO39 (SENSOR_VN) 不支持内部上拉/下拉，跳过配置
        if (IMU_INT_PIN != 39) {
            rtc_gpio_pullup_en((gpio_num_t)IMU_INT_PIN);
            rtc_gpio_pulldown_dis((gpio_num_t)IMU_INT_PIN);
        } else {
            Serial.println("[电源管理] ⚠️ GPIO39不支持内部上拉，需要外部上拉电阻");
        }

        // 在配置唤醒前检查GPIO39的稳定性
        if (IMU_INT_PIN == 39) {
            if (!checkGPIO39Stability()) {
                Serial.println("[电源管理] ❌ GPIO39状态不稳定，取消休眠");
                return false;
            }
        }

        // 配置EXT0唤醒
        ret = esp_sleep_enable_ext0_wakeup((gpio_num_t)IMU_INT_PIN, 0);
        if (ret != ESP_OK)
        {
            Serial.printf("[电源管理] ❌ EXT0唤醒配置失败: %s\n", esp_err_to_name(ret));
            return false;
        }

        Serial.printf("[电源管理] ✅ IMU唤醒配置成功 (GPIO%d)\n", IMU_INT_PIN);

        // 配置IMU
        extern IMU imu;
        if (!imu.configureForDeepSleep())
        {
            Serial.println("[电源管理] ❌ IMU深度睡眠配置失败");
            return false;
        }
        Serial.println("[电源管理] ✅ IMU已配置为深度睡眠模式");
    }
#endif

    // 3. 配置定时器唤醒（小时）
    const uint64_t BACKUP_WAKEUP_TIME = 60 * 60 * 1000000ULL;
    esp_err_t ret = esp_sleep_enable_timer_wakeup(BACKUP_WAKEUP_TIME);
    if (ret != ESP_OK)
    {
        Serial.printf("[电源管理] ❌ 定时器唤醒配置失败: %s\n", esp_err_to_name(ret));
        return false;
    }
    Serial.println("[电源管理] ✅ 定时器唤醒配置成功");

// 3. 配置车辆电门启动检测唤醒 (RTC_INT_PIN)
#ifdef RTC_INT_PIN
    if (RTC_INT_PIN >= 0 && RTC_INT_PIN <= 21)
    {
        // 检查是否为有效的RTC GPIO
        if (!rtc_gpio_is_valid_gpio((gpio_num_t)RTC_INT_PIN))
        {
            Serial.printf("[电源管理] ❌ 车辆电门检测引脚GPIO%d 不是有效的RTC GPIO\n", RTC_INT_PIN);
        }
        else
        {
            // 解除GPIO保持状态
            rtc_gpio_hold_dis((gpio_num_t)RTC_INT_PIN);

            // 初始化RTC GPIO
            esp_err_t ret = rtc_gpio_init((gpio_num_t)RTC_INT_PIN);
            if (ret != ESP_OK)
            {
                Serial.printf("[电源管理] ❌ 车辆电门检测RTC GPIO初始化失败: %s\n", esp_err_to_name(ret));
            }
            else
            {
                // 配置GPIO方向和上拉（车辆未启动时为高电平）
                rtc_gpio_set_direction((gpio_num_t)RTC_INT_PIN, RTC_GPIO_MODE_INPUT_ONLY);
                rtc_gpio_pullup_en((gpio_num_t)RTC_INT_PIN);
                rtc_gpio_pulldown_dis((gpio_num_t)RTC_INT_PIN);

                // 检查当前车辆电门状态
                int current_vehicle_state = digitalRead(RTC_INT_PIN);
                Serial.printf("[电源管理] 当前车辆电门状态: %s (GPIO%d = %d)\n", 
                             current_vehicle_state == LOW ? "已启动(供电中)" : "未启动", 
                             RTC_INT_PIN, current_vehicle_state);

                // 配置EXT1唤醒 - 当车辆启动时(从高电平变为低电平)唤醒
                // 使用EXT1可以支持多个引脚，这里只用一个引脚
                uint64_t ext1_mask = 1ULL << RTC_INT_PIN;
                ret = esp_sleep_enable_ext1_wakeup(ext1_mask, ESP_EXT1_WAKEUP_ALL_LOW);
                if (ret != ESP_OK)
                {
                    Serial.printf("[电源管理] ❌ 车辆电门唤醒配置失败: %s\n", esp_err_to_name(ret));
                }
                else
                {
                    Serial.printf("[电源管理] ✅ 车辆电门唤醒配置成功 (GPIO%d, 低电平触发)\n", RTC_INT_PIN);
                }
            }
        }
    }
#endif

    return true;
}

void PowerManager::disablePeripherals()
{
    // ===== 第一阶段：高级通信协议关闭 =====
    Serial.println("[电源管理] 开始第一阶段：关闭通信协议...");
    Serial.flush();
    delay(50);

    // 1. WiFi关闭
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    Serial.println("[电源管理] WiFi已安全关闭");
    Serial.flush();
    delay(50);

    // 2. 蓝牙关闭
    btStop();
    Serial.println("[电源管理] 蓝牙已安全关闭");
    Serial.flush();
    delay(50);

    // ===== 第二阶段：外设任务停止 =====
    Serial.println("[电源管理] 开始第二阶段：停止外设任务...");
    Serial.flush();
    delay(50);

#if defined(MODE_ALLINONE) || defined(MODE_SERVER)
    // GPS任务停止
    extern TaskHandle_t gpsTaskHandle;
    if (gpsTaskHandle != NULL)
    {
        vTaskDelete(gpsTaskHandle);
        gpsTaskHandle = NULL;
        Serial.println("[电源管理] GPS任务已停止");
        Serial.flush();
        delay(50);
    }

    // GPS串口关闭
#ifdef GPS_RX_PIN
    Serial2.end();
    pinMode(GPS_RX_PIN, INPUT);
    pinMode(GPS_TX_PIN, INPUT);
    Serial.println("[电源管理] GPS串口已关闭");
    Serial.flush();
    delay(50);
#endif
#endif

    // ===== 第三阶段：显示和LED关闭 =====
    Serial.println("[电源管理] 开始第三阶段：关闭显示和LED...");
    Serial.flush();
    delay(50);

#ifdef MODE_ALLINONE
    extern void tft_sleep();
    tft_sleep();

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
    digitalWrite(TFT_BL, LOW);
#endif

    Serial.println("[电源管理] 显示屏已完全关闭");
    Serial.flush();
    delay(50);
#endif

    // ===== 第四阶段：传感器和I2C关闭 =====
    Serial.println("[电源管理] 开始第四阶段：关闭传感器和I2C...");
    Serial.flush();
    delay(50);

#if defined(MODE_ALLINONE) || defined(MODE_SERVER)
    // 先关闭I2C总线
    Wire.end();
    Wire1.end();
    Serial.println("[电源管理] I2C总线已关闭");
    Serial.flush();
    delay(50);

    // 关闭其他I2C设备的引脚
#ifdef GPS_COMPASS_SDA
    pinMode(GPS_COMPASS_SDA, INPUT);
#endif
#ifdef GPS_COMPASS_SCL
    pinMode(GPS_COMPASS_SCL, INPUT);
#endif

    Serial.println("[电源管理] I2C设备已配置为低功耗模式");
    Serial.flush();
    delay(50);
#endif

    // ===== SD卡睡眠准备 =====
#ifdef ENABLE_SDCARD
    Serial.println("[电源管理] 开始SD卡睡眠准备...");
    Serial.flush();
    delay(50);
    
    extern SDManager sdManager;
    if (sdManager.isInitialized()) {
        // 记录进入睡眠的日志
        sdManager.writeLog("设备进入深度睡眠模式");
        // 准备SD卡进入睡眠
        sdManager.prepareForSleep();
        Serial.println("[电源管理] SD卡已准备进入睡眠");
    }
    Serial.flush();
    delay(50);
#endif

    // ===== 第五阶段：ADC和按钮配置 =====
    Serial.println("[电源管理] 开始第五阶段：配置ADC和按钮...");
    Serial.flush();
    delay(50);

    // ADC配置
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_11);

#ifdef BAT_PIN
    pinMode(BAT_PIN, INPUT);
#endif

    // 按钮配置
#ifdef BTN_PIN
    pinMode(BTN_PIN, INPUT_PULLUP);
#endif

    Serial.println("[电源管理] ADC和按钮已配置完成");
    Serial.flush();
    delay(50);

    // ===== 最后阶段：关闭时钟和降低CPU频率 =====
    Serial.println("[电源管理] 开始最后阶段：关闭时钟和降低CPU频率...");
    Serial.flush();
    delay(100);

    // 关闭不必要的外设时钟
    periph_module_disable(PERIPH_LEDC_MODULE);
    periph_module_disable(PERIPH_I2S0_MODULE);
    periph_module_disable(PERIPH_I2S1_MODULE);
    periph_module_disable(PERIPH_UART1_MODULE);
    periph_module_disable(PERIPH_UART2_MODULE);

    Serial.println("[电源管理] 外设时钟已关闭，准备进入深度睡眠...");
    Serial.println("[电源管理] 💤 即将进入深度睡眠模式");
    Serial.flush();
    delay(500); // 确保最后的消息能够发送完成
    
    // 完全关闭串口以避免乱码
    Serial.end();
    
    // 不降低CPU频率，直接进入深度睡眠
    // setCpuFrequencyMhz(10); // 注释掉这行，避免串口乱码
}

void PowerManager::loop()
{
    static unsigned long lastMotionCheck = 0;
    static unsigned long lastVehicleCheck = 0;
    unsigned long now = millis();

    // 每隔1秒检测一次车辆状态
    if (now - lastVehicleCheck >= 1000)
    {
        lastVehicleCheck = now;
        handleVehicleStateChange();
    }

    // 只每隔200ms检测一次运动
    if (now - lastMotionCheck >= 200)
    {
        lastMotionCheck = now;

#ifndef ENABLE_SLEEP
        // 如果编译时禁用了休眠功能，始终保持在正常状态
        if (powerState != POWER_STATE_NORMAL)
        {
            powerState = POWER_STATE_NORMAL;
        }
        return;
#else

        // 优化：当车辆启动时，跳过IMU运动检测
#ifdef RTC_INT_PIN
        bool vehicle_started = (digitalRead(RTC_INT_PIN) == LOW);
        if (vehicle_started) {
            // 车辆启动时，直接更新运动时间，跳过IMU检测
            lastMotionTime = millis();
            if (powerState != POWER_STATE_NORMAL) {
                powerState = POWER_STATE_NORMAL;
                Serial.println("[电源管理] 车辆启动，设备保持正常工作状态");
            }
            return; // 直接返回，跳过后续的IMU运动检测
        }
#endif

#ifdef ENABLE_IMU
        // 仅在正常工作状态下且车辆未启动时检测运动和空闲状态
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
                                  get_device_state()->battery_percentage,
                                  get_device_state()->battery_voltage);

                    enterLowPowerMode();
                }
            }
        }
#endif
#endif
    }
    // 其他低频逻辑可以继续执行
}

bool PowerManager::isDeviceIdle()
{
    // 注意：车辆启动状态检测已在loop()中处理，此函数只需检查空闲时间
    // 检查设备是否空闲足够长的时间
    return (millis() - lastMotionTime) > idleThreshold;
}

void PowerManager::enterLowPowerMode()
{
#ifdef MODE_CLIENT
    Serial.println("[电源管理] 客户端模式不使用睡眠功能");
    return;
#endif

#ifndef ENABLE_SLEEP
    Serial.println("[电源管理] 休眠功能已在编译时禁用");
    return;
#else
    if (!sleepEnabled)
    {
        Serial.println("[电源管理] 休眠功能已禁用");
        return;
    }

    Serial.println("[电源管理] 正在进入低功耗模式...");
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

    // 1. 先配置唤醒源（在关闭外设之前）
    Serial.println("[电源管理] ⏸️ 配置唤醒源...");
    if (!configureWakeupSources())
    {
        Serial.println("[电源管理] ❌ 唤醒源配置失败，终止休眠流程");
        powerState = POWER_STATE_NORMAL;
        return;
    }
    Serial.println("[电源管理] ✅ 唤醒源配置完成");

    // 2. 关闭外设
    Serial.println("[电源管理] ⏸️ 开始关闭外设...");
    disablePeripherals();
    Serial.println("[电源管理] ✅ 外设关闭完成");

    // 3. 配置电源域
    Serial.println("[电源管理] ⏸️ 配置电源域...");
    configurePowerDomains();
    Serial.println("[电源管理] ✅ 电源域配置完成");

    // 4. 最后的准备和信息输出
    Serial.println("[电源管理] 🌙 准备进入深度睡眠...");
#if defined(ENABLE_IMU) && defined(IMU_INT_PIN)
    Serial.printf("[电源管理] - IMU中断引脚: GPIO%d\n", IMU_INT_PIN);
#endif
    Serial.printf("[电源管理] - 定时器唤醒: 1小时\n");
    Serial.println("[电源管理] - 系统将在3秒后进入深度睡眠");
    Serial.flush();
    
    // 给足够时间让串口输出完成
    delay(3000);
    
    // 最后关闭串口
    Serial.end();

    // 5. 进入深度睡眠
    esp_deep_sleep_start();
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
 * GPIO39稳定性检查
 * 由于GPIO39不支持内部上拉，需要检查信号稳定性
 */
bool PowerManager::checkGPIO39Stability()
{
    Serial.println("[电源管理] 🔍 开始GPIO39稳定性检查...");
    
    // 连续检查10次，每次间隔50ms
    int stableCount = 0;
    int lastState = digitalRead(39);
    
    Serial.printf("[电源管理] GPIO39初始状态: %d\n", lastState);
    
    for(int i = 0; i < 10; i++) {
        delay(50);
        int currentState = digitalRead(39);
        
        if(currentState == lastState) {
            stableCount++;
        } else {
            Serial.printf("[电源管理] GPIO39状态变化: %d -> %d (第%d次检查)\n", 
                         lastState, currentState, i+1);
            stableCount = 0; // 重置稳定计数
            lastState = currentState;
        }
    }
    
    // 需要至少8次连续稳定读取
    bool isStable = (stableCount >= 8);
    
    Serial.printf("[电源管理] GPIO39稳定性检查结果: %s (稳定次数: %d/10)\n", 
                 isStable ? "稳定" : "不稳定", stableCount);
    
    if (!isStable) {
        Serial.println("[电源管理] ⚠️ GPIO39信号不稳定，可能需要外部上拉电阻");
        Serial.println("[电源管理] ⚠️ 建议添加10kΩ上拉电阻到3.3V");
    }
    
    return isStable;
}

/**
 * 检查车辆是否启动（电门开启）
 */
bool PowerManager::isVehicleStarted()
{
#ifdef RTC_INT_PIN
    return (digitalRead(RTC_INT_PIN) == LOW);
#else
    return false;
#endif
}

/**
 * 处理车辆状态变化
 */
void PowerManager::handleVehicleStateChange()
{
#ifdef RTC_INT_PIN
    static bool last_vehicle_state = false;
    static bool first_check = true;
    bool current_vehicle_state = isVehicleStarted();
    
    // 首次检查时记录当前状态，但不输出变化日志
    if (first_check) {
        last_vehicle_state = current_vehicle_state;
        first_check = false;
        if (current_vehicle_state) {
            Serial.println("[电源管理] 🚗 检测到车辆电门已启动");
            Serial.println("[电源管理] 将跳过IMU运动检测，直接保持活跃状态");
        }
        return;
    }
    
    if (current_vehicle_state != last_vehicle_state) {
        if (current_vehicle_state) {
            Serial.println("[电源管理] 🚗 车辆电门启动检测到！");
            Serial.println("[电源管理] 设备将保持活跃状态");
            Serial.println("[电源管理] ⚡ 优化：跳过IMU运动检测，节省CPU资源");
            // 重置运动时间，防止进入休眠
            lastMotionTime = millis();
            // 如果正在倒计时，取消进入休眠
            if (powerState == POWER_STATE_COUNTDOWN) {
                interruptLowPowerMode();
                Serial.println("[电源管理] 车辆启动，取消休眠倒计时");
            }
        } else {
            Serial.println("[电源管理] 🚗 车辆电门关闭检测到");
            Serial.println("[电源管理] 设备将根据运动状态决定是否休眠");
            Serial.println("[电源管理] ⚡ 恢复：重新启用IMU运动检测");
            // 重置运动时间，开始新的空闲计时
            lastMotionTime = millis();
        }
        last_vehicle_state = current_vehicle_state;
    }
#endif
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
#if defined(ENABLE_IMU) && defined(IMU_INT_PIN)
        if (IMU_INT_PIN >= 0 && IMU_INT_PIN <= 21)
        {
            Serial.printf("[系统] 从IMU运动检测唤醒 (GPIO%d)\n", IMU_INT_PIN);
        }
        else
        {
            Serial.println("[系统] 从外部RTC_IO唤醒，但IMU引脚配置无效");
        }
#endif
#ifdef RTC_INT_PIN
        Serial.printf("[系统] 可能从RTC外部电源唤醒 (GPIO%d)\n", RTC_INT_PIN);
#endif
        break;
    case ESP_SLEEP_WAKEUP_EXT1:
        Serial.println("[系统] 从外部RTC_CNTL唤醒");
#ifdef RTC_INT_PIN
        // 检查是否是车辆电门启动唤醒
        if (digitalRead(RTC_INT_PIN) == LOW) {
            Serial.printf("[系统] 🚗 检测到车辆电门启动！(GPIO%d = LOW)\n", RTC_INT_PIN);
            Serial.println("[系统] 车辆开始供电，设备从休眠中唤醒");
        } else {
            Serial.printf("[系统] EXT1唤醒，但车辆电门未启动 (GPIO%d = HIGH)\n", RTC_INT_PIN);
        }
#endif
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
#if defined(ENABLE_IMU) && defined(IMU_INT_PIN)
        if (IMU_INT_PIN >= 0 && IMU_INT_PIN <= 21)
        {
            if (digitalRead(IMU_INT_PIN) == LOW)
            {
                Serial.println("[系统] IMU运动唤醒检测到，记录运动事件");
                lastMotionTime = millis(); // 重置运动时间
            }
            else
            {
                Serial.println("[系统] 检测到按钮唤醒");
            }
        }
#endif
        break;
    case ESP_SLEEP_WAKEUP_EXT1:
        {
            Serial.println("[系统] 通过外部中断唤醒 (EXT1)");
#ifdef RTC_INT_PIN
            // 检查车辆电门启动状态
            bool vehicle_started = (digitalRead(RTC_INT_PIN) == LOW);
            if (vehicle_started) {
                Serial.println("[系统] 🚗 车辆电门启动检测到！");
                Serial.println("[系统] 设备将保持唤醒状态，直到车辆关闭");
                // 重置运动时间，防止立即进入休眠
                lastMotionTime = millis();
            } else {
                Serial.println("[系统] EXT1唤醒，但车辆电门未启动");
            }
#endif
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
void PowerManager::setSleepTime(unsigned long seconds)
{
    get_device_state()->sleep_time = seconds; // 更新设备状态
    sleepTimeSec = seconds;
    idleThreshold = sleepTimeSec * 1000;
    lastMotionTime = millis(); // 新增：重置空闲计时
    PreferencesUtils::saveSleepTime(sleepTimeSec);
    Serial.printf("[电源管理] 休眠时间已更新并保存: %lu 秒\n", sleepTimeSec);
}

// 新增：获取休眠时间（秒）
unsigned long PowerManager::getSleepTime() const
{
    if (get_device_state()->sleep_time != sleepTimeSec)
    {
        Serial.printf("[电源管理] 出现问题：休眠时间不一致 %lu 秒，device: %lu 秒\n", sleepTimeSec, get_device_state()->sleep_time);
    }
    return sleepTimeSec;
}
