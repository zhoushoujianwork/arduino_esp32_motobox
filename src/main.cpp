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
#include "led/LEDManager.h"
#include "device.h"

// 条件包含 MQTT 管理器头文件
#if (defined(ENABLE_GSM) || defined(ENABLE_WIFI)) && !defined(DISABLE_MQTT)
#include "net/MqttManager.h"
#endif

#ifdef BAT_PIN
#include "bat/BAT.h"
#endif

#ifdef RTC_INT_PIN
#include "power/ExternalPower.h"
#endif

#ifdef BTN_PIN
#include "btn/BTN.h"
#endif

#ifdef LED_PIN
#include "led/LED.h"
#endif

#ifdef ENABLE_COMPASS
#include "compass/Compass.h"
#endif

#ifdef ENABLE_GPS
#include "gps/GPS.h"
#ifdef USE_ML307_GPS
#include "net/Ml307Gps.h"
#endif
#endif

#ifdef ENABLE_IMU
#include "imu/qmi8658.h"
#endif

#ifdef ENABLE_SDCARD
#include "SD/SDManager.h"
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

#include "gps/GPSManager.h"

#include <SD.h> // SD卡库
#include <FS.h>

//============================= 全局变量 =============================

// RTC内存变量（深度睡眠后保持）
RTC_DATA_ATTR int bootCount = 0;

#ifdef ENABLE_SDCARD
// SD卡管理器
SDManager sdManager;
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
    pwmLed.loop();
#endif

#ifdef ENABLE_WIFI
    wifiManager.loop();
#endif

    // 电池监控
#ifdef BAT_PIN
    bat.loop();
#endif

    // 外部电源检测
#ifdef RTC_INT_PIN
    externalPower.loop();
#endif

    // SD卡状态监控
#ifdef ENABLE_SDCARD
    static unsigned long lastSDCheckTime = 0;
    unsigned long currentTime = millis();
    // 每10秒更新一次SD卡状态
    if (currentTime - lastSDCheckTime > 10000) {
        lastSDCheckTime = currentTime;
        if (sdManager.isInitialized()) {
            device_state.sdCardFreeMB = sdManager.getFreeSpaceMB();
        }
    }
#endif

    // 按钮状态更新
#ifdef BTN_PIN
    button.loop();
    BTN::handleButtonEvents();
#endif

    // 电源管理 - 始终保持处理
    powerManager.loop();

    // LED状态更新
    ledManager.loop();

    delay(5);
  }
}

/**
 * 数据处理任务
 * 负责数据采集、发送和显示
 */
void taskDataProcessing(void *parameter)
{
  Serial.println("[系统] 数据处理任务启动");
  
  // 数据记录相关变量
  unsigned long lastGPSRecordTime = 0;
  unsigned long lastIMURecordTime = 0;
  const unsigned long GPS_RECORD_INTERVAL = 5000;  // 5秒记录一次GPS数据
  const unsigned long IMU_RECORD_INTERVAL = 1000;  // 1秒记录一次IMU数据

  while (true)
  {
    // IMU数据处理
#ifdef ENABLE_IMU
    imu.setDebug(false);
    imu.loop();
#endif

    // GPS数据处理 - 使用统一的GPS管理器
    gpsManager.loop();
    // 更新GPS状态到设备状态
    device_state.gpsReady = gpsManager.isReady();

#ifdef ENABLE_SDCARD
    // 数据记录到SD卡
    unsigned long currentTime = millis();
    
    // 记录GPS数据
    if (device_state.gpsReady && device_state.sdCardReady && 
        currentTime - lastGPSRecordTime >= GPS_RECORD_INTERVAL) {
      lastGPSRecordTime = currentTime;
      
      // 获取GPS数据
      gps_data_t& gpsData = gps_data;
      
      // 获取当前时间作为时间戳
      char timestamp[32];
      sprintf(timestamp, "%04d-%02d-%02d %02d:%02d:%02d", 
              gpsData.year, gpsData.month, gpsData.day,
              gpsData.hour, gpsData.minute, gpsData.second);
      
      // 记录到SD卡
      sdManager.recordGPSData(
        gpsData.latitude, gpsData.longitude, gpsData.altitude,
        gpsData.speed, gpsData.satellites
      );
    }
    
    // 记录IMU数据
    if (device_state.imuReady && device_state.sdCardReady && 
        currentTime - lastIMURecordTime >= IMU_RECORD_INTERVAL) {
      lastIMURecordTime = currentTime;
      
      // 获取IMU数据
      float ax = imu.getAccelX();
      float ay = imu.getAccelY();
      float az = imu.getAccelZ();
      float gx = imu.getGyroX();
      float gy = imu.getGyroY();
      float gz = imu.getGyroZ();
      
      // 获取罗盘数据
      float heading = 0;
#ifdef ENABLE_COMPASS
      heading = compass_data.heading;
#endif
      
      // 获取当前时间作为时间戳
      char timestamp[32];
      time_t now;
      // IMU数据记录功能已简化，暂时移除
      // 可以在后续版本中根据需要重新添加
    }
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

    // MQTT 消息处理 - 条件编译
#ifndef DISABLE_MQTT
    mqttManager.loop();
#endif

    delay(10); // 增加延时，减少CPU占用
  }
}

#ifdef ENABLE_WIFI
// WiFi处理任务
void taskWiFi(void *parameter)
{
  Serial.println("[系统] WiFi任务启动");
  while (true)
  {
    if (wifiManager.getConfigMode())
    {
      wifiManager.loop();
    }
    delay(1); // 更短的延迟，提高响应性
  }
}
#endif

void setup()
{
  Serial.begin(115200);
  delay(100);

  PreferencesUtils::init();

  Serial.println("step 1");
  bootCount++;
  Serial.println("[系统] 启动次数: " + String(bootCount));

  Serial.println("step 2");
  powerManager.begin();

  Serial.println("step 3");
  powerManager.printWakeupReason();

  Serial.println("step 4");
  powerManager.checkWakeupCause();

  Serial.println("step 5");
  device.begin();

  //================ SD卡初始化开始 ================
#ifdef ENABLE_SDCARD
  Serial.println("step 6");
  if (sdManager.begin()) {
    Serial.println("[SD] SD卡初始化成功");
    
    // 更新设备状态
    device_state.sdCardReady = true;
    device_state.sdCardSizeMB = sdManager.getTotalSpaceMB();
    device_state.sdCardFreeMB = sdManager.getFreeSpaceMB();
    
    // 保存设备信息
    sdManager.saveDeviceInfo();
    
    Serial.println("[SD] 设备信息已保存到SD卡");
  } else {
    Serial.println("[SD] SD卡初始化失败");
    device_state.sdCardReady = false;
  }
#endif
  //================ SD卡初始化结束 ================

  // 创建任务
  xTaskCreate(taskSystem, "TaskSystem", 1024 * 15, NULL, 1, NULL);
  xTaskCreate(taskDataProcessing, "TaskData", 1024 * 15, NULL, 2, NULL);
#ifdef ENABLE_WIFI
  xTaskCreate(taskWiFi, "TaskWiFi", 1024 * 15, NULL, 3, NULL);
#endif

  Serial.println("[系统] 初始化完成");
}

void loop()
{
  // 主循环留空，所有功能都在RTOS任务中处理
  delay(20);

  // 每 30 秒发送一次状态消息
  static unsigned long lastMsg = 0;
  if (millis() - lastMsg > 1000)
  {
    lastMsg = millis();

    // 打印设备 ID
    Serial.println("设备 ID: " + String(device.get_device_id()));

    update_device_state();

    bat.print_voltage();

    // 获取GPS数据用于调试
    if (gpsManager.isReady())
    {
      print_lbs_data(lbs_data);
      print_gps_data(gps_data);
    }

    // 显示LBS基站定位信息 - 通过GPS管理器获取
    // if (gpsManager.isLBSReady()) {
    //   lbs_data_t lbsData = gpsManager.getLBSData();
    //   if (lbsData.valid) {
    //     Serial.printf("[LBS] 位置: %.6f, %.6f, 半径: %d m\n",
    //                  lbsData.latitude, lbsData.longitude, lbsData.radius);
    //   }
    // }

    // 发送状态消息 - 条件编译
#ifndef DISABLE_MQTT
    // String status = String("设备运行时间: ") + (millis() / 1000) + "秒";
    // mqttManager.publish("test/status", status.c_str());
    // mqttManager.publishToTopic("device_info", device_state_to_json(&device_state).c_str());
#endif
  }
}
