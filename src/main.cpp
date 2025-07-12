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

// 函数声明
void handleSerialCommand();

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

#ifdef ENABLE_AUDIO
// 音频管理器
AudioManager audioManager;
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
        currentTime - lastGNSSRecordTime >= GNSS_RECORD_INTERVAL)
    {
      lastGNSSRecordTime = currentTime;

      // 获取GPS数据
      gps_data_t &gpsData = gps_data;

      // 获取当前时间作为时间戳
      char timestamp[32];
      sprintf(timestamp, "%04d-%02d-%02d %02d:%02d:%02d",
              gpsData.year, gpsData.month, gpsData.day,
              gpsData.hour, gpsData.minute, gpsData.second);

      // 记录到SD卡
      sdManager.recordGPSData(
          gpsData.latitude, gpsData.longitude, gpsData.altitude,
          gpsData.speed, gpsData.satellites);
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
  }

  delay(20);

  // static unsigned long lastMsg = 0;
  // if (millis() - lastMsg > 30000)
  // {
  //   lastMsg = millis();
  //   Serial.println("[状态] 定期状态更新开始...");
  //   update_device_state();
  //   // GNSS数据更新等已注释
  //   Serial.println("[状态] 定期状态更新完成");
  // }
}

// ===================== 串口命令处理函数 =====================
/**
 * 处理串口输入命令
 * 支持SD卡、GSM、MQTT、音频等命令
 */
void handleSerialCommand()
{
  String command = Serial.readStringUntil('\n');
  command.trim();

  if (command.length() > 0)
  {
    Serial.println(">>> 收到命令: " + command);

#ifdef ENABLE_SDCARD
    // SD卡相关命令
    if (command.startsWith("sd."))
    {
      sdManager.handleSerialCommand(command);
    }
    else
#endif
      // Air780EG调试命令
      if (command.startsWith("gsm."))
      {
#ifdef USE_AIR780EG_GSM
        if (command == "gsm.test")
        {
          Serial.println("=== Air780EG 测试 ===");
          Serial.printf("GSM_EN引脚状态: %s\n", digitalRead(GSM_EN) ? "HIGH" : "LOW");
          Serial.printf("RX引脚: %d, TX引脚: %d\n", GSM_RX_PIN, GSM_TX_PIN);
          Serial.printf("模块状态: %s\n", device_state.gsmReady ? "就绪" : "未就绪");

          // 尝试发送AT命令
          Serial.println("发送AT命令测试...");
          if (device_state.gsmReady)
          {
            String response = air780eg.getCore().sendATCommand("AT");
            Serial.printf("AT响应: %s\n", response.c_str());

            response = air780eg.getCore().sendATCommand("ATI");
            Serial.printf("模块信息: %s\n", response.c_str());
          }
          else
          {
            Serial.println("模块未就绪，无法测试AT命令");
          }
        }
        else if (command == "gsm.reset")
        {
          Serial.println("重置Air780EG模块...");
          digitalWrite(GSM_EN, LOW);
          delay(1000);
          digitalWrite(GSM_EN, HIGH);
          delay(2000);
          Serial.println("重置完成");
        }
        else if (command == "gsm.info")
        {
          Serial.println("=== Air780EG 信息 ===");
          Serial.printf("模块状态: %s\n", device_state.gsmReady ? "就绪" : "未就绪");
          if (device_state.gsmReady)
          {
            Serial.printf("网络状态: %s\n", air780eg.getNetwork().isNetworkRegistered() ? "已连接" : "未连接");
            Serial.printf("信号强度: %d dBm\n", air780eg.getNetwork().getSignalStrength());
            Serial.printf("运营商: %s\n", air780eg.getNetwork().getOperatorName().c_str());
            Serial.printf("网络类型: %s\n", air780eg.getNetwork().getNetworkType().c_str());
            Serial.printf("IMEI: %s\n", air780eg.getNetwork().getIMEI().c_str());

            // GNSS信息
            if (air780eg.getGNSS().isFixed())
            {
              Serial.printf("GNSS状态: 已定位\n");
              Serial.printf("位置: %.6f, %.6f\n", air780eg.getGNSS().getLatitude(), air780eg.getGNSS().getLongitude());
              Serial.printf("卫星数: %d\n", air780eg.getGNSS().getSatelliteCount());
            }
            else
            {
              Serial.printf("GNSS状态: 未定位 (卫星数: %d)\n", air780eg.getGNSS().getSatelliteCount());
            }
          }
        }
        else if (command == "gsm.mqtt")
        {
          Serial.println("=== Air780EG MQTT测试 ===");
          if (device_state.gsmReady)
          {
            // 测试MQTT功能支持
            // mqttManager.testMQTTSupport();
            Serial.println("MQTT功能已禁用");
          }
          else
          {
            Serial.println("GSM模块未就绪，无法测试MQTT功能");
          }
        }
        else if (command == "gsm.mqtt.debug")
        {
          Serial.println("=== Air780EG MQTT连接调试 ===");
          if (device_state.gsmReady)
          {
            // TODO: 实现基于新库的MQTT调试功能
            Serial.println("MQTT调试功能正在重构中...");
            air780eg.printStatus();
          }
          else
          {
            Serial.println("GSM模块未就绪，无法进行MQTT调试");
          }
        }
        else if (command == "gsm.lbs")
        {
          Serial.println("=== Air780EG LBS调试 ===");
          if (device_state.gsmReady)
          {
            // TODO: 实现基于新库的LBS功能
            Serial.println("LBS功能正在重构中...");
            if (air780eg.getGNSS().isFixed())
            {
              Serial.printf("当前位置 (GNSS): %.6f, %.6f\n",
                            air780eg.getGNSS().getLatitude(),
                            air780eg.getGNSS().getLongitude());
            }
            else
            {
              Serial.println("GNSS未定位，无法提供位置信息");
            }
          }
          else
          {
            Serial.println("GSM模块未就绪，无法调试LBS功能");
          }
        }
        else if (command == "gsm.lbs.test")
        {
          Serial.println("=== Air780EG LBS AT命令测试 ===");
          if (device_state.gsmReady)
          {
            // TODO: 实现基于新库的LBS AT命令测试
            Serial.println("LBS AT命令测试功能正在重构中...");
            Serial.println("当前网络信息:");
            Serial.printf("- 网络注册: %s\n", air780eg.getNetwork().isNetworkRegistered() ? "是" : "否");
            Serial.printf("- 信号强度: %d dBm\n", air780eg.getNetwork().getSignalStrength());
            Serial.printf("- 运营商: %s\n", air780eg.getNetwork().getOperatorName().c_str());
          }
          else
          {
            Serial.println("GSM模块未就绪，无法测试LBS AT命令");
          }
        }
#else
      Serial.println("Air780EG模块未启用");
#endif
      }
      else if (command == "info")
      {
        Serial.println("=== 设备信息 ===");
        Serial.println("设备ID: " + device_state.device_id);
        Serial.println("固件版本: " + String(device_state.device_firmware_version));
        Serial.println("硬件版本: " + String(device_state.device_hardware_version));
        Serial.println("启动次数: " + String(bootCount));
        Serial.println("运行时间: " + String(millis() / 1000) + " 秒");
        Serial.println("");
        Serial.println("--- 连接状态 ---");
        Serial.println("WiFi状态: " + String(device_state.wifiConnected ? "已连接" : "未连接"));
        Serial.println("BLE状态: " + String(device_state.bleConnected ? "已连接" : "未连接"));
#ifdef ENABLE_GSM
        Serial.println("GSM状态: " + String(device_state.gsmReady ? "就绪" : "未就绪"));
#ifdef USE_AIR780EG_GSM
        if (device_state.gsmReady)
        {
          Serial.println("网络状态: " + String(air780eg.getNetwork().isNetworkRegistered() ? "已连接" : "未连接"));
          Serial.println("信号强度: " + String(air780eg.getNetwork().getSignalStrength()) + " dBm");
          Serial.println("运营商: " + air780eg.getNetwork().getOperatorName());
          if (air780eg.getGNSS().isFixed())
          {
            Serial.println("GNSS状态: 已定位 (卫星数: " + String(air780eg.getGNSS().getSatelliteCount()) + ")");
          }
          else
          {
            Serial.println("GNSS状态: 未定位 (卫星数: " + String(air780eg.getGNSS().getSatelliteCount()) + ")");
          }
        }
#endif

        // MQTT连接状态和配置信息
#ifndef DISABLE_MQTT
        Serial.println("MQTT状态: " + air780eg.getMQTT().getState());
        Serial.println("MQTT服务器: " + String(MQTT_BROKER) + ":" + String(MQTT_PORT));

        // 显示已注册的主题
        Serial.println("--- MQTT主题配置 ---");
        String deviceId = device_state.device_id;
        String baseTopic = "vehicle/v1/" + deviceId;
        Serial.println("基础主题: " + baseTopic);
        Serial.println("设备信息: " + baseTopic + "/telemetry/device");
        Serial.println("位置信息: " + baseTopic + "/telemetry/location");
        Serial.println("运动信息: " + baseTopic + "/telemetry/motion");
        Serial.println("控制命令: " + baseTopic + "/ctrl/#");
#else
        Serial.println("MQTT功能: ❌ 已禁用");
#endif
#endif
        Serial.println("");
        Serial.println("--- 传感器状态 ---");
        Serial.println("GNSS状态: " + String(device_state.gnssReady ? "就绪" : "未就绪"));
        Serial.println("IMU状态: " + String(device_state.imuReady ? "就绪" : "未就绪"));
        Serial.println("罗盘状态: " + String(device_state.compassReady ? "就绪" : "未就绪"));
        Serial.println("");
        Serial.println("--- 音频设备状态 ---");
#ifdef ENABLE_AUDIO
        Serial.println("音频系统: " + String(device_state.audioReady ? "✅ 就绪" : "❌ 未就绪"));
        if (device_state.audioReady)
        {
          Serial.printf("音频引脚: WS=%d, BCLK=%d, DATA=%d\n", IIS_S_WS_PIN, IIS_S_BCLK_PIN, IIS_S_DATA_PIN);
          Serial.println("音频芯片: NS4168 功率放大器");
          Serial.println("采样率: 16kHz");
          Serial.println("位深度: 16位");
          Serial.println("声道: 单声道");
          Serial.println("支持事件: 开机音/WiFi连接音/GPS定位音/低电量音/睡眠音");
        }
        else
        {
          Serial.println("⚠️ 音频系统未就绪，请检查:");
          Serial.println("  1. I2S引脚连接是否正确");
          Serial.println("  2. NS4168芯片是否正常工作");
          Serial.println("  3. 音频引脚是否与其他功能冲突");
        }
#else
      Serial.println("音频系统: ❌ 未启用 (编译时禁用)");
#endif
        Serial.println("");
        Serial.println("--- 电源状态 ---");
        Serial.println("电池电压: " + String(device_state.battery_voltage) + " mV");
        Serial.println("电池电量: " + String(device_state.battery_percentage) + "%");
        Serial.println("充电状态: " + String(device_state.is_charging ? "充电中" : "未充电"));
        Serial.println("外部电源: " + String(device_state.external_power ? "已连接" : "未连接"));
        Serial.println("");
#ifdef ENABLE_SDCARD
        Serial.println("--- SD卡状态 ---");
        if (device_state.sdCardReady)
        {
          Serial.println("SD卡状态: 就绪");
          Serial.println("SD卡容量: " + String((unsigned long)device_state.sdCardSizeMB) + " MB");
          Serial.println("SD卡剩余: " + String((unsigned long)device_state.sdCardFreeMB) + " MB");
        }
        else
        {
          Serial.println("SD卡状态: 未就绪");
          Serial.println("⚠️ 请检查SD卡是否正确插入");
        }
#endif
      }
      else if (command == "status")
      {
        Serial.println("=== 系统状态 ===");
        Serial.println("系统正常运行");
        Serial.println("空闲内存: " + String(ESP.getFreeHeap()) + " 字节");
        Serial.println("最小空闲内存: " + String(ESP.getMinFreeHeap()) + " 字节");
        Serial.println("芯片温度: " + String(temperatureRead(), 1) + "°C");
        Serial.println("CPU频率: " + String(ESP.getCpuFreqMHz()) + " MHz");
      }
      else if (command.startsWith("mqtt."))
      {
#ifndef DISABLE_MQTT
        if (command == "mqtt.status")
        {
          Serial.println("=== MQTT状态 ===");
          Serial.println("MQTT服务器: " + String(MQTT_BROKER));
          Serial.println("MQTT端口: " + String(MQTT_PORT));
          Serial.println("保持连接: " + String(MQTT_KEEPALIVE) + "秒");

#ifdef USE_AIR780EG_GSM
          Serial.println("连接方式: Air780EG GSM");
          Serial.println("GSM状态: " + String(device_state.gsmReady ? "就绪" : "未就绪"));
          if (device_state.gsmReady)
          {
            Serial.println("网络状态: " + String(air780eg.getNetwork().isNetworkRegistered() ? "已连接" : "未连接"));
            Serial.println("信号强度: " + String(air780eg.getNetwork().getSignalStrength()) + " dBm");
            Serial.println("运营商: " + air780eg.getNetwork().getOperatorName());
          }
#elif defined(ENABLE_WIFI)
          Serial.println("连接方式: WiFi");
          Serial.println("WiFi状态: " + String(device_state.wifiConnected ? "已连接" : "未连接"));
#endif
          // MQTT连接状态
          Serial.println("MQTT连接: " + air780eg.getMQTT().getState());
        }
        else
        {
          Serial.println("未知MQTT命令，输入 'mqtt.help' 查看帮助");
        }
#else
      Serial.println("MQTT功能已禁用");
#endif
      }
      else if (command.startsWith("audio."))
      {
#ifdef ENABLE_AUDIO
        if (command == "audio.test")
        {
          Serial.println("=== 音频系统测试 ===");
          if (device_state.audioReady)
          {
            Serial.println("播放测试音频序列...");
            audioManager.testAudio();
          }
          else
          {
            Serial.println("❌ 音频系统未就绪，无法测试");
          }
        }
        else if (command == "audio.boot")
        {
          Serial.println("播放开机成功音...");
          if (device_state.audioReady)
          {
            audioManager.playBootSuccessSound();
          }
          else
          {
            Serial.println("❌ 音频系统未就绪");
          }
        }
        else if (command == "audio.wifi")
        {
          Serial.println("播放WiFi连接音...");
          if (device_state.audioReady)
          {
            audioManager.playWiFiConnectedSound();
          }
          else
          {
            Serial.println("❌ 音频系统未就绪");
          }
        }
        else if (command == "audio.gps")
        {
          Serial.println("播放GPS定位音...");
          if (device_state.audioReady)
          {
            audioManager.playGPSFixedSound();
          }
          else
          {
            Serial.println("❌ 音频系统未就绪");
          }
        }
        else if (command == "audio.battery")
        {
          Serial.println("播放低电量音...");
          if (device_state.audioReady)
          {
            audioManager.playLowBatterySound();
          }
          else
          {
            Serial.println("❌ 音频系统未就绪");
          }
        }
        else if (command == "audio.sleep")
        {
          Serial.println("播放睡眠音...");
          if (device_state.audioReady)
          {
            audioManager.playSleepModeSound();
          }
          else
          {
            Serial.println("❌ 音频系统未就绪");
          }
        }
        else if (command == "audio.help")
        {
          Serial.println("=== 音频命令帮助 ===");
          Serial.println("audio.test    - 播放测试音频序列");
          Serial.println("audio.boot    - 播放开机成功音");
          Serial.println("audio.wifi    - 播放WiFi连接音");
          Serial.println("audio.gps     - 播放GPS定位音");
          Serial.println("audio.battery - 播放低电量音");
          Serial.println("audio.sleep   - 播放睡眠音");
          Serial.println("audio.help    - 显示此帮助信息");
        }
        else
        {
          Serial.println("未知音频命令，输入 'audio.help' 查看帮助");
        }
#else
      Serial.println("音频功能未启用");
#endif
      }
      else if (command == "restart" || command == "reboot")
      {
        Serial.println("正在重启设备...");
        Serial.flush();
        delay(1000);
        ESP.restart();
      }
      else if (command == "help")
      {
        Serial.println("=== 可用命令 ===");
        Serial.println("基本命令:");
        Serial.println("  info     - 显示详细设备信息");
        Serial.println("  status   - 显示系统状态");
        Serial.println("  restart  - 重启设备");
        Serial.println("  help     - 显示此帮助信息");
        Serial.println("");
#ifdef ENABLE_SDCARD
        Serial.println("SD卡命令:");
        Serial.println("  sd.info    - 显示SD卡详细信息");
        Serial.println("  sd.test    - 测试GPS数据记录");
        Serial.println("  sd.status  - 检查SD卡状态");
        Serial.println("  sd.session - 显示当前GPS会话信息");
        Serial.println("  sd.finish  - 结束当前GPS会话");
        Serial.println("  sd.dirs    - 检查和创建目录结构");
        Serial.println("");
#endif
#ifdef USE_AIR780EG_GSM
        Serial.println("Air780EG命令:");
        Serial.println("  gsm.test   - 测试AT命令和波特率");
        Serial.println("  gsm.reset  - 重置Air780EG模块");
        Serial.println("  gsm.info   - 显示模块状态信息");
        Serial.println("  gsm.mqtt   - 测试MQTT功能支持");
        Serial.println("  gsm.mqtt.debug - MQTT连接详细调试");
        Serial.println("  gsm.lbs    - LBS基站定位调试");
        Serial.println("  gsm.lbs.test - LBS AT命令测试");
        Serial.println("  gps.status - SimpleGPS 状态查询");
        Serial.println("");
#endif
        Serial.println("提示: 命令不区分大小写");
      }
      else
      {
        Serial.println("❌ 未知命令: " + command);
        Serial.println("输入 'help' 查看可用命令");
      }

    Serial.println(""); // 添加空行分隔
  }
}
