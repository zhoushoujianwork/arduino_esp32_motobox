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
#include "config.h"
#include "power/PowerManager.h"
#include "device.h"
#include "nvs_flash.h"

#if defined(ENABLE_GSM) || defined(ENABLE_WIFI)
#include "ml370/MqttManager.h"
#endif

#ifdef BAT_PIN
#include "bat/BAT.h"
#endif

#ifdef BTN_PIN
#include "btn/BTN.h"
#endif

#ifdef LED_PIN
#include "led/LED.h"
#endif

#ifdef PWM_LED_PIN
#include "led/PWMLED.h"
#endif

#ifdef ENABLE_COMPASS
#include "compass/Compass.h"
#endif

#ifdef ENABLE_GPS
#include "gps/GPS.h"
#endif

#ifdef ENABLE_IMU
#include "qmi8658/IMU.h"
#endif

#include "version.h"
#ifdef BLE_CLIENT
#include "ble/ble_client.h"
#endif
#ifdef BLE_SERVER
#include "ble/ble_server.h"
#endif

// 仅在未定义DISABLE_TFT时包含TFT相关头文件
#ifdef ENABLE_TFT
#include "tft/TFT.h"
#endif
//============================= 全局变量 =============================

// RTC内存变量（深度睡眠后保持）
RTC_DATA_ATTR int bootCount = 0;

// 任务句柄
TaskHandle_t gpsTaskHandle = NULL;

/**
 * GPS任务
 * 负责GPS数据获取和处理
 */
#ifdef ENABLE_GPS
void taskGps(void *parameter)
{
  while (true)
  {
    // 使用printRawData直接打印原始NMEA数据
    // gps.printRawData();
    gps.loop();
    delay(5);
  }
}
#endif

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
#include "led/PWMLED.h"
    pwmLed.loop();
#endif

#ifdef LED_PIN
#include "led/LED.h"
    led.setMode(isConnected ? LED::BLINK_DUAL : LED::BLINK_5_SECONDS);
#endif

// 电池监控
#ifdef BAT_PIN
    bat.loop();
#endif

// LED状态设置
// - WiFi连接时显示黄色
// - MQTT连接时显示蓝色
// - 无连接时显示红色
#ifdef PWM_LED_PIN
    if (device_state.wifiConnected)
    {
      pwmLed.setMode(PWMLED::GREEN, 5);
      // } else if (device.get_device_state()->bleConnected) {
      //     pwmLed.setMode(PWMLED::GREEN);
    }
    else
    {
      pwmLed.setMode(PWMLED::RED, 10);
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
// IMU数据处理
#ifdef ENABLE_IMU
    imu.loop();
#endif

#ifdef BLE_CLIENT
    // 蓝牙客户端处理
    bc.loop();
#endif

#ifdef BLE_SERVER
    bs.loop();
#endif

#ifdef ENABLE_TFT
    // 显示屏更新
    tft_loop();
#endif

#ifdef ENABLE_COMPASS
    // 罗盘数据处理
    compass.loop();
#endif

    delay(5);
  }
}

void setup()
{
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
  {
    nvs_flash_erase();
    nvs_flash_init();
  }
  Serial.begin(115200);
  delay(100);

  Serial.println("step 1");
  bootCount++;
  Serial.println("[系统] 启动次数: " + String(bootCount));

  Serial.println("step 2");
  powerManager.begin();

  Serial.println("step 3");
  powerManager.printWakeupReason();

  Serial.println("step 3");
  powerManager.checkWakeupCause();

  Serial.println("step 4");
  device.begin();

#ifdef ENABLE_GPS
  xTaskCreatePinnedToCore(taskGps, "TaskGps", 1024 * 10, NULL, 1, &gpsTaskHandle, 1);
#endif

  xTaskCreate(taskSystem, "TaskSystem", 1024 * 10, NULL, 2, NULL);
  xTaskCreate(taskDataProcessing, "TaskData", 1024 * 10, NULL, 1, NULL);

  Serial.println("[系统] 初始化完成");
}

void loop()
{
  // 主循环留空，所有功能都在RTOS任务中处理
  delay(20);
  // MQTT 消息处理
  mqttManager.loop();
  device.loop();

  // 每 30 秒发送一次状态消息
  static unsigned long lastMsg = 0;
  if (millis() - lastMsg > 30000)
  {
    lastMsg = millis();

    // 获取网络信息
    String networkInfo = mqttManager.getNetworkInfo();
    Serial.println("网络状态: " + networkInfo);

    // 发送状态消息
    String status = String("设备运行时间: ") + (millis() / 1000) + "秒";
    mqttManager.publish("test/status", status.c_str());
  }
}
