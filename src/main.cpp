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

#include "config.h"
#include "Arduino.h"
#include "config.h"
#include "power/PowerManager.h"
#include "led/LEDManager.h"
#include "device.h"
#include "Air780EG.h"
#include "utils/serialCommand.h"

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

// GSM模块包含
#ifdef USE_AIR780EG_GSM
// 直接使用新的Air780EG库
#include <Air780EG.h>

#elif defined(USE_ML307_GSM)
#include "net/Ml307AtModem.h"
extern Ml307AtModem ml307_at;
#endif

#ifdef ENABLE_IMU
#include "imu/qmi8658.h"
#endif

#ifdef ENABLE_SDCARD
#include "SD/SDManager.h"
#endif

#ifdef ENABLE_AUDIO
#include "audio/AudioManager.h"
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

    // 串口命令处理
    if (Serial.available())
    {
      handleSerialCommand();
    }

    // 外部电源检测
#ifdef RTC_INT_PIN
    externalPower.loop();
#endif

    // SD卡状态监控
#ifdef ENABLE_SDCARD
    static unsigned long lastSDCheckTime = 0;
    unsigned long currentTime = millis();
    // 每10秒更新一次SD卡状态
    if (currentTime - lastSDCheckTime > 10000)
    {
      lastSDCheckTime = currentTime;
      if (sdManager.isInitialized())
      {
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
  unsigned long lastGNSSRecordTime = 0;
  unsigned long lastIMURecordTime = 0;

  for (;;)
  {
    // Air780EG库处理 - 必须在主循环中调用
#ifdef USE_AIR780EG_GSM
    // 调用新库的主循环（处理URC、网络状态更新、GNSS数据更新等）
    air780eg.loop();
#endif

    // IMU数据处理
#ifdef ENABLE_IMU
    imu.setDebug(false);
    imu.loop();
#endif

#ifdef ENABLE_SDCARD
    // 数据记录到SD卡
    unsigned long currentTime = millis();

    // 记录GPS数据
    if (device_state.gnssReady && device_state.sdCardReady &&
        currentTime - lastGNSSRecordTime >= 1000)
    {
      lastGNSSRecordTime = currentTime;

      // 记录到SD卡
      sdManager.recordGPSData(
          air780eg.getGNSS().gnss_data);
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

    delay(10); // 增加延时，减少CPU占用
  } // for循环结束
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
  delay(1000);

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
  if (sdManager.begin())
  {
    Serial.println("[SD] SD卡初始化成功");

    // 更新设备状态
    device_state.sdCardReady = true;
    device_state.sdCardSizeMB = sdManager.getTotalSpaceMB();
    device_state.sdCardFreeMB = sdManager.getFreeSpaceMB();

    // 保存设备信息
    sdManager.saveDeviceInfo();

    Serial.println("[SD] 设备信息已保存到SD卡");
  }
  else
  {
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

#ifdef ENABLE_AUDIO
  Serial.println("音频功能: ✅ 编译时已启用");
  if (device_state.audioReady)
  {
    audioManager.playBootSuccessSound();
  }
#else
  Serial.println("音频功能: ❌ 编译时未启用");
#endif
  Serial.println("=== 系统初始化完成 ===");
}

void loop()
{
  static unsigned long loopCount = 0;
  static unsigned long lastLoopReport = 0;
  loopCount++;

  if (millis() - lastLoopReport > 10000)
  {
    lastLoopReport = millis();
    uint32_t freeHeap = ESP.getFreeHeap();
    uint32_t minFreeHeap = ESP.getMinFreeHeap();
    Serial.printf("[主循环] 运行正常，循环计数: %lu, 空闲内存: %d 字节\n", loopCount, freeHeap);
    if (freeHeap < 50000)
    {
      Serial.printf("[警告] 内存不足: %d 字节，最小值: %d 字节\n", freeHeap, minFreeHeap);
    }
    if (freeHeap < 20000)
    {
      Serial.println("[严重] 内存严重不足，即将重启系统...");
      delay(1000);
      ESP.restart();
    }
    // air780eg.getGNSS().printGNSSInfo();
    printCompassData();
  }

  delay(20);
}