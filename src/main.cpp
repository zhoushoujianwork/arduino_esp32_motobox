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

// GSM模块包含
#ifdef USE_AIR780EG_GSM
#include "net/Air780EGModem.h"
extern Air780EGModem air780eg_modem;
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
    if (Serial.available()) {
        String command = Serial.readStringUntil('\n');
        command.trim();
        
        if (command.length() > 0) {
            Serial.println(">>> 收到命令: " + command);
            
#ifdef ENABLE_SDCARD
            // SD卡相关命令
            if (command.startsWith("sd.")) {
                sdManager.handleSerialCommand(command);
            }
            else
#endif
            if (command == "info") {
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
                Serial.println("");
                Serial.println("--- 传感器状态 ---");
                Serial.println("GPS状态: " + String(device_state.gpsReady ? "就绪" : "未就绪"));
                Serial.println("IMU状态: " + String(device_state.imuReady ? "就绪" : "未就绪"));
                Serial.println("");
                Serial.println("--- 电源状态 ---");
                Serial.println("电池电压: " + String(device_state.battery_voltage) + " mV");
                Serial.println("电池电量: " + String(device_state.battery_percentage) + "%");
                Serial.println("充电状态: " + String(device_state.is_charging ? "充电中" : "未充电"));
                Serial.println("外部电源: " + String(device_state.external_power ? "已连接" : "未连接"));
                Serial.println("");
#ifdef ENABLE_SDCARD
                Serial.println("--- SD卡状态 ---");
                if (device_state.sdCardReady) {
                    Serial.println("SD卡状态: 就绪");
                    Serial.println("SD卡容量: " + String((unsigned long)device_state.sdCardSizeMB) + " MB");
                    Serial.println("SD卡剩余: " + String((unsigned long)device_state.sdCardFreeMB) + " MB");
                } else {
                    Serial.println("SD卡状态: 未就绪");
                    Serial.println("⚠️ 请检查SD卡是否正确插入");
                }
#endif
            }
            else if (command == "status") {
                Serial.println("=== 系统状态 ===");
                Serial.println("系统正常运行");
                Serial.println("空闲内存: " + String(ESP.getFreeHeap()) + " 字节");
                Serial.println("最小空闲内存: " + String(ESP.getMinFreeHeap()) + " 字节");
                Serial.println("芯片温度: " + String(temperatureRead(), 1) + "°C");
                Serial.println("CPU频率: " + String(ESP.getCpuFreqMHz()) + " MHz");
            }
            else if (command == "restart" || command == "reboot") {
                Serial.println("正在重启设备...");
                Serial.flush();
                delay(1000);
                ESP.restart();
            }
            else if (command == "help") {
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
                Serial.println("提示: 命令不区分大小写");
            }
            else {
                Serial.println("❌ 未知命令: " + command);
                Serial.println("输入 'help' 查看可用命令");
            }
            
            Serial.println(""); // 添加空行分隔
        }
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

  // 调试信息：检查各系统初始化状态
  Serial.println("=== 系统初始化状态检查 ===");
  Serial.printf("音频系统状态: %s\n", device_state.audioReady ? "✅ 就绪" : "❌ 未就绪");
  Serial.printf("WiFi状态: %s\n", device_state.wifiConnected ? "✅ 已连接" : "❌ 未连接");
  Serial.printf("GPS状态: %s\n", device_state.gpsReady ? "✅ 就绪" : "❌ 未就绪");
  Serial.printf("IMU状态: %s\n", device_state.imuReady ? "✅ 就绪" : "❌ 未就绪");
  
#ifndef DISABLE_MQTT
  Serial.println("MQTT功能: ✅ 已启用");
#else
  Serial.println("MQTT功能: ❌ 已禁用");
#endif

#ifdef ENABLE_AUDIO
  Serial.println("音频功能: ✅ 编译时已启用");
  if (device_state.audioReady) {
    Serial.println("尝试播放测试音频...");
    audioManager.playBootSuccessSound();
  }
#else
  Serial.println("音频功能: ❌ 编译时未启用");
#endif
  Serial.println("=== 系统初始化状态检查结束 ===");

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

  // 宏定义测试
  Serial.println("=== 宏定义测试 ===");
#ifdef ENABLE_AIR780EG
  Serial.println("✅ ENABLE_AIR780EG 已定义");
#else
  Serial.println("❌ ENABLE_AIR780EG 未定义");
#endif

#ifdef USE_AIR780EG_GSM
  Serial.println("✅ USE_AIR780EG_GSM 已定义");
#else
  Serial.println("❌ USE_AIR780EG_GSM 未定义");
#endif

#ifdef ENABLE_GSM
  Serial.println("✅ ENABLE_GSM 已定义");
#else
  Serial.println("❌ ENABLE_GSM 未定义");
#endif
  Serial.println("=== 宏定义测试结束 ===");

  //================ GSM模块初始化开始 ================
#ifdef USE_AIR780EG_GSM
  Serial.println("step 6.5");
  Serial.println("[GSM] 初始化Air780EG模块...");
  
  Serial.printf("[GSM] 引脚配置 - RX:%d, TX:%d, EN:%d\n", GSM_RX_PIN, GSM_TX_PIN, GSM_EN);
  
  air780eg_modem.setDebug(true);
  if (air780eg_modem.begin()) {
    Serial.println("[GSM] ✅ Air780EG基础初始化成功");
    device_state.gsmReady = true;
    
    // 检查GSM_EN引脚状态
    Serial.printf("[GSM] GSM_EN引脚状态: %s\n", digitalRead(GSM_EN) ? "HIGH" : "LOW");
    
    Serial.println("[GSM] 📡 网络注册和GNSS启用将在后台任务中完成");
  } else {
    Serial.println("[GSM] ❌ Air780EG基础初始化失败");
    device_state.gsmReady = false;
    
    // 调试信息
    Serial.printf("[GSM] GSM_EN引脚状态: %s\n", digitalRead(GSM_EN) ? "HIGH" : "LOW");
  }
#endif
  //================ GSM模块初始化结束 ================

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
  // 添加循环计数器用于调试
  static unsigned long loopCount = 0;
  static unsigned long lastLoopReport = 0;
  loopCount++;
  
  // 每10秒报告一次循环状态
  if (millis() - lastLoopReport > 10000) {
    lastLoopReport = millis();
    Serial.printf("[主循环] 运行正常，循环计数: %lu, 空闲内存: %d 字节\n", 
                  loopCount, ESP.getFreeHeap());
  }

  // Air780EG后台初始化处理
#ifdef USE_AIR780EG_GSM
  air780eg_modem.loop();
#endif

  // 主循环留空，所有功能都在RTOS任务中处理
  delay(20);

  // 每 30 秒发送一次状态消息
  static unsigned long lastMsg = 0;
  if (millis() - lastMsg > 30000)  // 改为30秒发送一次，减少频率
  {
    lastMsg = millis();

    Serial.println("[状态] 定期状态更新开始...");

    update_device_state();

    // Air780EG状态检查和GNSS数据更新
#ifdef USE_AIR780EG_GSM
    static unsigned long lastGNSSCheck = 0;
    if (millis() - lastGNSSCheck > 5000) {  // 每5秒检查一次
      lastGNSSCheck = millis();
      
      // 更新GNSS数据
      if (air780eg_modem.updateGNSSData()) {
        GNSSData gnss = air780eg_modem.getGNSSData();
        if (gnss.valid) {
          Serial.printf("[GNSS] 位置: %.6f, %.6f, 卫星: %d\n", 
                       gnss.latitude, gnss.longitude, gnss.satellites);
          
          // 更新设备状态
          device_state.gpsReady = true;
          device_state.latitude = gnss.latitude;
          device_state.longitude = gnss.longitude;
          device_state.satellites = gnss.satellites;
        } else {
          device_state.gpsReady = false;
        }
      }
      
      // 网络状态检查
      if (air780eg_modem.isNetworkReady()) {
        device_state.gsmReady = true;
        device_state.signalStrength = air780eg_modem.getCSQ();
      } else {
        device_state.gsmReady = false;
      }
    }
#endif

    // 获取GPS数据用于调试
    if (gpsManager.isReady())
    {
      print_lbs_data(lbs_data);
      print_gps_data(gps_data);
    }

    // 发送状态消息 - 条件编译
#ifndef DISABLE_MQTT
    Serial.println("[MQTT] 准备发送状态消息...");
    String status = String("设备运行时间: ") + (millis() / 1000) + "秒";
    bool publishResult1 = mqttManager.publish("test/status", status.c_str());
    bool publishResult2 = mqttManager.publishToTopic("device_info", device_state_to_json(&device_state).c_str());
    Serial.printf("[MQTT] 状态消息发送结果: status=%s, device_info=%s\n", 
                  publishResult1 ? "成功" : "失败", 
                  publishResult2 ? "成功" : "失败");
#endif
    
    Serial.println("[状态] 定期状态更新完成");
  }
}
