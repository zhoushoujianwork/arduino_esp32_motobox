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

BTN button(BTN_PIN);
MQTT mqtt(MQTT_SERVER, MQTT_PORT, MQTT_USER, MQTT_PASSWORD);
#endif

#ifdef MODE_CLIENT
BLEC bc;
#endif

#ifdef MODE_SERVER
BLES bs;
#endif

GPS gps(GPS_RX_PIN, GPS_TX_PIN);
IMU imu(IMU_SDA_PIN, IMU_SCL_PIN);
BAT bat(BAT_PIN, BAT_MIN_VOLTAGE, BAT_MAX_VOLTAGE); // 电池电压 3.3V - 4.2V
LED led(LED_PIN); // 假设LED连接在GPIO 8上

void task0(void *parameter)
{
  static int hzs[] = {1, 2, 5, 10};
  static int hz = 0;

  while (true)
  {
    led.loop();
    bat.loop();
    // 优化电压打印频率
    static uint32_t lastBatPrint = 0;
    if (millis() - lastBatPrint > MQTT_BAT_PRINT_INTERVAL)
    {
      bat.print_voltage();
      lastBatPrint = millis();
    }

#if defined(MODE_ALLINONE) || defined(MODE_SERVER)

    gps.loop();
    imu.loop();

    // 优化状态判断逻辑
    bool isConnected =
        device.get_mqtt_connected() && device.get_wifi_connected();
    led.setMode(isConnected ? LED::BLINK_DUAL : LED::OFF);

    // 检查点击
    if (button.isClicked())
    {
      hz = (hz + 1) % 4;
      delay(100); // 保持短暂延时确保任务挂起
      gps.setGpsHz(hzs[hz]);
      Serial.printf("设置GPS更新率为%dHz\n", hzs[hz]);
      device.get_device_state()->gpsHz = hzs[hz];
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

#if defined(MODE_ALLINONE) || defined(MODE_CLIENT)
    tft_loop();
#endif

    delay(10); // 保持原有延时
  }
}

void task1(void *parameter)
{
  const TickType_t xDelay = 1000 / portTICK_PERIOD_MS; // 添加任务周期控制
  while (true)
  {

#if defined(MODE_ALLINONE) || defined(MODE_SERVER)
    wifiManager.loop();
    mqtt.loop();
#endif

#ifdef MODE_CLIENT
    bc.loop();
#endif

#ifdef MODE_SERVER
    bs.loop();
#endif

    vTaskDelay(xDelay); // 重要！添加任务延时防止阻塞
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
  // debug的信息
  Serial.printf("free heap: %d\n", ESP.getFreeHeap());

  device.printImuData();

  device.printGpsData();

  device.print_device_info();

  // 打印自己的电池电压信息
  bat.print_voltage();

  delay(1000);
}