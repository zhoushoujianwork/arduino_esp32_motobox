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
GPS gps(GPS_RX_PIN, GPS_TX_PIN);
BTN btn(BTN_PIN);

void setup()
{
  Serial.begin(115200);
  gps.begin();

  Serial.println("main setup end");
}

int hz = 0;
const int hzs[] = {1, 2, 5, 10};

// 时间戳
unsigned long timestamp = 0;

void loop()
{
  btn.loop();
  gps.printRawData();
  // 1s 打印一次时间戳
  if (millis() - timestamp > 1000)
  {
    Serial.println(timestamp);
    Serial.println();
    Serial.println();
    timestamp = millis();
  }

  // 检查点击
  if (btn.isClicked())
  {
    Serial.println("检测到点击");
    hz = (hz + 1) % 4;
    delay(100);
    if (gps.setGpsHz(hzs[hz]))
    {
        Serial.printf("设置GPS更新率为%dHz\n", hzs[hz]);
      }
      else
      {
      Serial.println("设置GPS更新率失败");
    }
  }

  delay(100);
}