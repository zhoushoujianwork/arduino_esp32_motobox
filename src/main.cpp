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
#include "power/PowerManager.h"
#include "qmi8658/IMU.h"
#include "wifi/WifiManager.h"
#include "compass/Compass.h"
#include "esp_wifi.h"
#include "version.h"

// 仅在未定义DISABLE_TFT时包含TFT相关头文件
#ifndef DISABLE_TFT
#include "tft/TFT.h"
#endif
//============================= 全局变量 =============================

// 设备管理实例
Device device;

// RTC内存变量（深度睡眠后保持）
RTC_DATA_ATTR int bootCount = 0;

// 任务句柄
TaskHandle_t gpsTaskHandle = NULL;
TaskHandle_t wifiTaskHandle = NULL;

// 发布时间记录
unsigned long lastGpsPublishTime = 0;
unsigned long lastImuPublishTime = 0;
unsigned long lastBlePublishTime = 0;
unsigned long lastDeviceInfoPublishTime = 0;
//============================= 设备实例 =============================

#if defined(MODE_ALLINONE) || defined(MODE_SERVER)
GPS gps(GPS_RX_PIN, GPS_TX_PIN);
IMU imu(IMU_SDA_PIN, IMU_SCL_PIN, IMU_INT_PIN);

MQTT mqtt(MQTT_SERVER, MQTT_PORT, MQTT_USER, MQTT_PASSWORD);
BLES bs;
#endif

#ifdef BTN_PIN
BTN button(BTN_PIN);
#endif

#ifdef MODE_CLIENT
BLEC bc;
#endif

BAT bat(BAT_PIN, BAT_MIN_VOLTAGE, BAT_MAX_VOLTAGE);

#ifdef PWM_LED_PIN
PWMLED pwmLed(PWM_LED_PIN);
#endif
#ifdef LED_PIN
    led.setMode(isConnected ? LED::BLINK_DUAL : LED::BLINK_5_SECONDS);
#endif

PowerManager powerManager;

#ifdef MODE_ALLINONE
Compass compass(GPS_COMPASS_SDA, GPS_COMPASS_SCL);
#endif

//============================= 函数声明 =============================

//============================= 任务定义 =============================

/**
 * WiFi管理任务
 * 负责WiFi连接和配置
 */
void taskWifi(void *parameter)
{
#if defined(MODE_ALLINONE) || defined(MODE_SERVER)
  while (true)
  {
    wifiManager.loop();
    delay(5);
  }
#endif
}

/**
 * GPS任务
 * 负责GPS数据获取和处理
 */
void taskGps(void *parameter)
{
#if defined(MODE_ALLINONE) || defined(MODE_SERVER)
  while (true)
  {
    // 使用printRawData直接打印原始NMEA数据
    // gps.printRawData();
    gps.loop();
    delay(5);
  }
#endif
}

/**
 * 系统监控任务
 * 负责电源管理、LED状态、按钮处理
 */
void taskSystem(void *parameter)
{
  // 添加任务启动提示
  Serial.println("[系统] 系统监控任务启动");

  while (true)
  {
// LED状态更新
#ifdef PWM_LED_PIN
    pwmLed.loop();
#endif
#ifdef LED_PIN
    led.setMode(isConnected ? LED::BLINK_DUAL : LED::BLINK_5_SECONDS);
#endif

// 电池监控
#ifdef MODE_CLIENT
    if (!device.get_device_state()->bleConnected)
    {
      bat.loop();
    }
#else
    bat.loop();
#endif

// LED状态设置
// - WiFi连接时显示黄色
// - MQTT连接时显示蓝色
// - 无连接时显示红色
#ifdef PWM_LED_PIN
    if (device.get_device_state()->wifiConnected) {
        pwmLed.setMode(PWMLED::GREEN,5);
    // } else if (device.get_device_state()->bleConnected) {
    //     pwmLed.setMode(PWMLED::GREEN);
    } else {
        pwmLed.setMode(PWMLED::RED,10);
    }
#endif

#ifdef LED_PIN
    led.setMode(isConnected ? LED::BLINK_DUAL : LED::BLINK_5_SECONDS);
#endif

    // 按钮状态更新
#ifdef BTN_PIN
    button.loop();
    BTN::handleButtonEvents();
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
void taskDataProcessing(void *parameter)
{
  while (true)
  {
#if defined(MODE_ALLINONE) || defined(MODE_SERVER)
    // IMU数据处理
    imu.loop();

    // MQTT数据发布
    mqtt.loop();

    // deviceinfo 数据发布 2000ms
    if (millis() - lastDeviceInfoPublishTime >= 2000)
    {
      mqtt.publishDeviceInfo(*device.get_device_state());
      device.print_device_info();
      lastDeviceInfoPublishTime = millis();
    }

    // GPS数据发布 (1Hz)
    if (millis() - lastGpsPublishTime >= 1000)
    {
      mqtt.publishGPS(*device.get_gps_data());
      // device.printGpsData();
      lastGpsPublishTime = millis();
      // 更新GPS就绪状态
      if (device.get_gps_data()->satellites > 3)
      {
        device.set_gps_ready(true);
        gps.autoAdjustHz(device.get_gps_data()->satellites);
      }
      else
      {
        device.set_gps_ready(false);
      }
    }

    // IMU数据发布 (2Hz)
    if (millis() - lastImuPublishTime >= 500)
    {
      mqtt.publishIMU(*device.get_imu_data());
      lastImuPublishTime = millis();
    }
#endif

#ifdef MODE_CLIENT
    // 蓝牙客户端处理
    bc.loop();
#endif

#ifdef MODE_SERVER
    // 蓝牙服务器广播 (1Hz)
    if (millis() - lastBlePublishTime >= 1000)
    {
      bs.loop();
      lastBlePublishTime = millis();
    }
#endif

#if defined(MODE_ALLINONE) || defined(MODE_CLIENT)
    // 显示屏更新
    tft_loop();
#endif

#if defined(MODE_ALLINONE)
    // 罗盘数据处理
    compass.loop();
#endif

    delay(5);
  }
}

//============================= 辅助函数 =============================

//============================= ARDUINO框架函数 =============================

void setup()
{
  // 初始化串口
  Serial.begin(115200);
  delay(100);

  // 增加启动计数并打印
  bootCount++;
  Serial.println("[系统] 启动次数: " + String(bootCount));

  // 打印唤醒原因
  powerManager.printWakeupReason();

  // 检查唤醒原因并处理
  powerManager.checkWakeupCause();

  // 初始化硬件
  device.initializeHardware();

#if defined(MODE_ALLINONE) || defined(MODE_SERVER)
  // 显示屏初始化完成后，再初始化 GPS
  Serial.println("[系统] 初始化GPS...");
  gps.begin();
#endif

// 创建任务
#if defined(MODE_ALLINONE) || defined(MODE_SERVER)
  xTaskCreate(taskWifi, "TaskWifi", 1024 * 10, NULL, 1, &wifiTaskHandle);
  xTaskCreatePinnedToCore(taskGps, "TaskGps", 1024 * 10, NULL, 1, &gpsTaskHandle, 1);
#endif

  xTaskCreate(taskSystem, "TaskSystem", 1024 * 10, NULL, 2, NULL);
  xTaskCreate(taskDataProcessing, "TaskData", 1024 * 10, NULL, 1, NULL);
  Serial.println("[系统] 初始化完成");
}

void loop()
{
  // 主循环留空，所有功能都在RTOS任务中处理
  delay(1000);
}
