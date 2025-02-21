#include "Arduino.h"
#include "bat/BAT.h"
#include "ble/ble_server.h"
#include "ble/ble_client.h"
#include "btn/BTN.h"
#include "config.h"
#include "device.h"
#include "gps/GPS.h"
#include "led/LED.h"
#include "mqtt/MQTT.h"
#include "tft/TFT.h"
#include "qmi8658/IMU.h"
#include "wifi/WifiManager.h"

Device device;

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

BAT bat(BAT_PIN, BAT_MIN_VOLTAGE, BAT_MAX_VOLTAGE); // 电池电压 3.3V - 4.2V
LED led(LED_PIN);                                   // 假设LED连接在GPIO 8上

// 在头部添加全局变量声明
unsigned long lastGpsPublishTime = 0;
unsigned long lastImuPublishTime = 0;
unsigned long lastBlePublishTime = 0;

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

// gps task 句柄
void taskGps(void *parameter)
{
#if defined(MODE_ALLINONE) || defined(MODE_SERVER)
  while (true)
  {
    gps.loop();
    // gps.printRawData();
    delay(5);
  }
#endif
}

void task0(void *parameter)
{
  static int hzs[] = {1, 2, 5, 10};
  static int hz = 0;

  while (true)
  {
    led.loop();
// 电池电压 当在 client 模式下，且蓝牙不连接的时候，记录电池电压
#ifdef MODE_CLIENT
    if (!device.get_device_state()->bleConnected)
    {
      bat.loop();
    }
#else
    bat.loop();
#endif

#if defined(MODE_ALLINONE) || defined(MODE_SERVER)
    // 优化状态判断逻辑
    bool isConnected =
        device.get_device_state()->mqttConnected && device.get_device_state()->wifiConnected;
    led.setMode(isConnected ? LED::BLINK_DUAL : LED::OFF);

    // 检查点击
    if (button.isClicked())
    {
      Serial.println("检测到点击");
      hz = (hz + 1) % 4;
      // vTaskSuspend(gpsTaskHandle);
      delay(1000); // 等待1秒, 确保挂起成功
      if (gps.setGpsHz(hzs[hz]))
      {
        Serial.printf("设置GPS更新率为%dHz\n", hzs[hz]);
      }
      else
      {
        Serial.println("设置GPS更新率失败");
      }
      // 恢复gps task
      // vTaskResume(gpsTaskHandle);
    }

    // 检查长按重置
    if (button.isLongPressed())
    {
      Serial.println("检测到长按,重置WIFI");
      if (!wifiManager.getConfigMode())
      {
        wifiManager.reset();
        // 重启设备
        ESP.restart();
      }
    }
#endif

    delay(5);
  }
}

void task1(void *parameter)
{

  while (true)
  {
#if defined(MODE_ALLINONE) || defined(MODE_SERVER)
    imu.loop();
    mqtt.loop();
    // mqtt数据发布，gps数据 1 秒一个，imu数据 500 毫秒一个
    if (millis() - lastGpsPublishTime >= 1000)
    {
      mqtt.publishGPS(*device.get_gps_data());
      lastGpsPublishTime = millis();
      // device.print_device_info();
      // gps数量超过3颗星，则认为gpsReady为true
      if (device.get_gps_data()->satellites > 3)
        device.set_gps_ready(true);
      else
        device.set_gps_ready(false);
    }
    if (millis() - lastImuPublishTime >= 500)
    {
      mqtt.publishIMU(*device.get_imu_data());
      lastImuPublishTime = millis();
    }

#endif

#ifdef MODE_CLIENT
    bc.loop();
#endif

#if defined(MODE_ALLINONE) || defined(MODE_CLIENT)
    tft_loop();
#endif

#ifdef MODE_SERVER
    // 1000ms 执行一次 主动广播
    if (millis() - lastBlePublishTime >= 1000)
    {
      bs.loop();
      lastBlePublishTime = millis();
    }
#endif

    delay(5);
  }
}

void setup()
{
  Serial.begin(115200);
  led.begin();
  led.setMode(LED::OFF);

  // 初始化设备
  device.init();

#if defined(MODE_ALLINONE) || defined(MODE_SERVER)
  gps.begin();
  imu.begin();
  wifiManager.begin();

  xTaskCreate(taskWifi, "TaskWifi", 1024 * 10, NULL, 0, NULL);
  xTaskCreate(taskGps, "TaskGps", 1024 * 10, NULL, 1, NULL);
#endif

#if defined(MODE_ALLINONE) || defined(MODE_CLIENT)
  tft_begin();
#endif

#ifdef MODE_SERVER
  bs.setup();
#endif

#ifdef MODE_CLIENT
  bc.setup();
#endif

  // esp双核任务
  xTaskCreate(task0, "Task0", 1024 * 10, NULL, 0, NULL);
  xTaskCreate(task1, "Task1", 1024 * 10, NULL, 1, NULL);

  Serial.println("main setup end");
}

void loop()
{
  // device.printImuData();
  // delay(10);

  // device.printGpsData();
  delay(500);
}