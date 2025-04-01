/**
 * ESP32 MotoBox - 摩托车数据采集与显示系统
 * 
 * 硬件：ESP32-S3
 * 版本：2.0.0
 * 
 * 模式：
 * - MODE_ALLINONE: 独立模式，所有功能集成在一个设备上
 * - MODE_SERVER: 服务器模式，采集数据并通过BLE发送给客户端，同时通过MQTT发送到服务器
 * - MODE_CLIENT: 客户端模式，通过BLE接收服务器数据并显示
 */

#include "Arduino.h"
#include "bat/BAT.h"
#include "ble/ble_server.h"
#include "ble/ble_client.h"
#include "btn/BTN.h"
#include "config.h"
#include "device.h"
#include "gps/GPS.h"
#include "led/LED.h"
#include "led/PWMLED.h"
#include "mqtt/MQTT.h"
#include "tft/TFT.h"
#include "qmi8658/IMU.h"
#include "wifi/WifiManager.h"
#include "power/PowerManager.h"

//============================= 全局变量 =============================

// 设备管理实例
Device device;

// RTC内存变量（深度睡眠后保持）
RTC_DATA_ATTR int bootCount = 0;

// 任务句柄
TaskHandle_t gpsTaskHandle = NULL;

// 发布时间记录
unsigned long lastGpsPublishTime = 0;
unsigned long lastImuPublishTime = 0;
unsigned long lastBlePublishTime = 0;

//============================= 设备实例 =============================

#if defined(MODE_ALLINONE) || defined(MODE_SERVER)
GPS gps(GPS_RX_PIN, GPS_TX_PIN);
IMU imu(IMU_SDA_PIN, IMU_SCL_PIN);
BTN button(BTN_PIN);

MQTT mqtt(MQTT_SERVER, MQTT_PORT, MQTT_USER, MQTT_PASSWORD);
#endif

#ifdef MODE_CLIENT
BLEC bc;
#endif

#ifdef MODE_SERVER
BLES bs;
#endif

BAT bat(BAT_PIN, BAT_MIN_VOLTAGE, BAT_MAX_VOLTAGE);

#ifdef PWM_LED_PIN
PWMLED pwmLed(PWM_LED_PIN);
#else
LED led(LED_PIN);
#endif

PowerManager powerManager;

//============================= 函数声明 =============================

/**
 * 按钮事件处理
 */
void handleButtonEvents();

/**
 * 打印唤醒原因
 */
void printWakeupReason();

/**
 * 初始化所有硬件和模块
 */
void initializeHardware();

//============================= 任务定义 =============================

/**
 * WiFi管理任务
 * 负责WiFi连接和配置
 */
void taskWifi(void *parameter) {
  #if defined(MODE_ALLINONE) || defined(MODE_SERVER)
  while (true) {
    wifiManager.loop();
    delay(5);
  }
  #endif
}

/**
 * GPS任务
 * 负责GPS数据获取和处理
 */
void taskGps(void *parameter) {
  #if defined(MODE_ALLINONE) || defined(MODE_SERVER)
  // 创建计时器以跟踪GPS数据解析
  unsigned long lastStatsTime = 0;
  
  while (true) {
    // 任选一种方式：
    // 1. 使用loop函数解析NMEA数据并更新设备GPS信息
    gps.loop();
    
    // 2. 或者使用printRawData直接打印原始NMEA数据（调试用）
    // gps.printRawData();
    
    // 定期打印GPS统计信息，显示接收质量
    // gps.printGpsStats();
    
    delay(5);
  }
  #endif 
}

/**
 * 系统监控任务
 * 负责电源管理、LED状态、按钮处理
 */
void taskSystem(void *parameter) {
  // GPS更新率自动调整的时间记录
  unsigned long lastGpsRateAdjustTime = 0;
  
  // 添加任务启动提示
  Serial.println("[系统] 系统监控任务启动");
 
  while (true) {
    // LED状态更新
    #ifdef PWM_LED_PIN
    static unsigned long lastLedDebugTime = 0;
    if (millis() - lastLedDebugTime >= 1000) {
        Serial.println("[系统] 执行PWM LED更新");
        lastLedDebugTime = millis();
    }
    pwmLed.loop();
    #else
    led.loop();
    #endif
    
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
    #ifdef PWM_LED_PIN
    pwmLed.setMode(isConnected ? PWMLED::BREATH : PWMLED::RAINBOW);
    #else
    led.setMode(isConnected ? LED::BLINK_DUAL : LED::OFF);
    #endif

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

    delay(5);
  }
}

/**
 * 数据处理任务
 * 负责数据采集、发送和显示
 */
void taskDataProcessing(void *parameter) {
  while (true) {
    #if defined(MODE_ALLINONE) || defined(MODE_SERVER)
    // IMU数据处理
    imu.loop();
    
    // MQTT数据发布
    mqtt.loop();
    
    // GPS数据发布 (1Hz)
    if (millis() - lastGpsPublishTime >= 1000) {
      mqtt.publishGPS(*device.get_gps_data());
      lastGpsPublishTime = millis();
      
      // 更新GPS就绪状态
      device.set_gps_ready(device.get_gps_data()->satellites > 3);
    }
    
    // IMU数据发布 (2Hz)
    if (millis() - lastImuPublishTime >= 500) {
      mqtt.publishIMU(*device.get_imu_data());
      lastImuPublishTime = millis();
    }
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
    if (millis() - lastBlePublishTime >= 1000) {
      bs.loop();
      lastBlePublishTime = millis();
    }
    #endif

    delay(5);
  }
}

//============================= 辅助函数 =============================

/**
 * 按钮事件处理
 */
void handleButtonEvents() {
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

/**
 * 打印唤醒原因
 */
void printWakeupReason() {
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason) {
    case ESP_SLEEP_WAKEUP_EXT0: {
      // 检查唤醒源是按钮还是IMU
      int wakeup_pin = -1;
      // 检查哪个引脚为低电平，可能是唤醒源
      if (BTN_PIN >= 0 && BTN_PIN <= 21 && digitalRead(BTN_PIN) == LOW) {
        wakeup_pin = BTN_PIN;
        Serial.printf("[系统] 从按钮唤醒 (GPIO%d)\n", BTN_PIN);
      } else if (IMU_INT1_PIN >= 0 && IMU_INT1_PIN <= 21 && digitalRead(IMU_INT1_PIN) == LOW) {
        wakeup_pin = IMU_INT1_PIN;
        Serial.printf("[系统] 从IMU运动检测唤醒 (GPIO%d)\n", IMU_INT1_PIN);
      } else {
        Serial.println("[系统] 从外部RTC_IO唤醒，但无法确定具体引脚");
      }
      break;
    }
    case ESP_SLEEP_WAKEUP_EXT1: 
      // ESP_SLEEP_WAKEUP_EXT1使用位掩码，但我们的代码没有使用此方式
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
  
  // 打印系统信息
  Serial.printf("[系统] ESP32-S3芯片ID: %llX\n", ESP.getEfuseMac());
  Serial.printf("[系统] 总运行内存: %d KB\n", ESP.getHeapSize() / 1024);
  Serial.printf("[系统] 可用运行内存: %d KB\n", ESP.getFreeHeap() / 1024);
  Serial.printf("[系统] CPU频率: %d MHz\n", ESP.getCpuFreqMHz());
}

/**
 * 初始化所有硬件和模块
 */
void initializeHardware() {
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
  #ifdef PWM_LED_PIN
  pwmLed.begin();
  pwmLed.setMode(PWMLED::OFF);
  #else
  led.begin();
  led.setMode(LED::OFF);
  #endif

  // 初始化设备
  device.init();

  #if defined(MODE_ALLINONE) || defined(MODE_SERVER)
  // IMU初始化
  Serial.println("[系统] 初始化IMU...");
  imu.begin();
  
  // 如果是从深度睡眠唤醒，检查唤醒原因
  if (isWakeFromDeepSleep) {
    switch (wakeup_reason) {
      case ESP_SLEEP_WAKEUP_EXT0:
        // 如果是按钮唤醒，立即激活LED指示
        if (BTN_PIN >= 0 && BTN_PIN <= 21 && digitalRead(BTN_PIN) == LOW) {
          #ifdef PWM_LED_PIN
          pwmLed.setMode(PWMLED::RED);
          #else
          led.setMode(LED::BLINK_SINGLE);
          #endif
          Serial.println("[系统] 按钮唤醒检测到，准备恢复系统");
        } 
        // 如果是IMU唤醒，处理运动事件
        else if (IMU_INT1_PIN >= 0 && IMU_INT1_PIN <= 21 && digitalRead(IMU_INT1_PIN) == LOW) {
          Serial.println("[系统] IMU运动唤醒检测到，记录运动事件");
          // 这里可以添加运动事件处理代码
          // 例如：记录唤醒时间、记录运动状态等
        }
        break;
      case ESP_SLEEP_WAKEUP_TIMER:
        Serial.println("[系统] 定时器唤醒，检查系统状态");
        // 这里可以添加定期状态检查代码
        break;
      default:
        break;
    }
  }
  
  // WiFi初始化
  Serial.println("[系统] 初始化WiFi连接...");
  wifiManager.begin();
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
  
  // 在TFT完全初始化后显示休眠功能状态通知
  delay(100); // 短暂延迟确保UI已准备好
  #if ENABLE_SLEEP
  tft_show_notification("休眠已启用", "1分钟无活动将进入休眠", 3000);
  #else
  tft_show_notification("休眠已禁用", "睡眠功能已关闭", 3000);
  #endif
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

//============================= ARDUINO框架函数 =============================

void setup() {
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
  
  #if defined(MODE_ALLINONE) || defined(MODE_SERVER)
  // 显示屏初始化完成后，再初始化 GPS
  Serial.println("[系统] 初始化GPS...");
  gps.begin();
  #endif
  
  // 创建任务
  #if defined(MODE_ALLINONE) || defined(MODE_SERVER)
  xTaskCreate(taskWifi, "TaskWifi", 1024 * 10, NULL, 1, NULL);
  xTaskCreatePinnedToCore(taskGps, "TaskGps", 1024 * 10, NULL, 1, &gpsTaskHandle, 1);
  #endif
  
  xTaskCreate(taskSystem, "TaskSystem", 1024 * 10, NULL, 2, NULL);
  xTaskCreate(taskDataProcessing, "TaskData", 1024 * 10, NULL, 1, NULL);

  Serial.println("[系统] 初始化完成");
}

void loop() {
  // 主循环留空，所有功能都在RTOS任务中处理
  delay(10000);
}