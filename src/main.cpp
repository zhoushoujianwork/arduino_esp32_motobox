#include <Arduino.h>
#include "btn/BTN.h"
#include "led/LED.h"
#include "bat/BAT.h"
#include "device.h"
#include "config.h"
#include "tft/TFT.h"
#include "wifi/WifiManager.h"
#include "mqtt/MQTT.h"

Device device;
#if Enable_BLE
#include "ble/ble.h"
BLE ble;
#endif

#if Enable_IMU
#include "QMI8658/imu.h"
IMU imu(42, 2);
#endif

#if Enable_GPS
#include "gps/GPS.h"
GPS gps(12, 11);
TaskHandle_t gpsTaskHandle;

void gpsTask(void *parameter)
{
    while (true)
    {
        gps.loop();
        // gps.printRawData();
#if Enable_IMU
        // imu.printImuData();
        imu.loop();
#endif
        delay(10);
    }
}
#endif

#if Enable_BAT
BAT bat(BAT_PIN, BAT_MIN_VOLTAGE, BAT_MAX_VOLTAGE); // 电池电压 3.3V - 4.2V
#endif

BTN button(BTN_PIN);
LED led(8); // 假设LED连接在GPIO 8上

// 创建MQTT实例
MQTT mqtt(MQTT_SERVER, MQTT_PORT, MQTT_USER, MQTT_PASSWORD);

void deviceTask(void *parameter)
{
    while (true)
    {
#if Enable_BAT
        bat.loop();
        bat.print_voltage();
        device.set_battery(bat.getPercentage());
#endif

        delay(2000);
    }
}

// 按钮任务
void btnTask(void *parameter)
{
    static int hzs[] = {2, 5, 10, 1}; // 支持的更新率
    static int hz = 0;                // 当前更新率索引

    while (true)
    {
        if (button.isClicked())
        {
#if Enable_GPS
            // 循环切换更新率
            hz = (hz + 1) % 4;
            // 暂停GPS任务
            vTaskSuspend(gpsTaskHandle);
            delay(100);

            // 循环切换更新率
            gps.setGpsHz(hzs[hz]);
            Serial.printf("设置GPS更新率为%dHz\n", hzs[hz]);

            // 恢复GPS任务
            vTaskResume(gpsTaskHandle);
#endif
            device.get_device_state()->gpsHz = hzs[hz];
        }

        // 检查长按重置
        if (button.isLongPressed())
        {
            Serial.println("检测到长按");
#if Enable_WIFI
            if (!wifiManager.getConfigMode())
            {
                wifiManager.reset();
                // 重启设备
                ESP.restart();
            }
#endif
        }

        led.loop();
        // device.print_device_info();
        if (device.get_mqtt_connected() && device.get_wifi_connected())
        {
            led.setMode(LED::BLINK_DUAL);
        }
        else
        {
            led.setMode(LED::OFF);
        }

#if Enable_TFT
        tft_loop();
#endif
        delay(10);
    }
}

#if Enable_WIFI
// 阻塞的wifi任务
void wifiTask(void *parameter)
{
    while (true)
    {

        wifiManager.loop();
        delay(10);
    }
}
#endif

void setup()
{
    Serial.begin(115200);
    led.begin();
    led.setMode(LED::OFF);

    wifiManager.begin();

#if Enable_IMU
    // 添加IMU初始化检查
    imu.begin();
#endif

#if Enable_GPS
    gps.begin();
#endif

#if Enable_BLE
    ble.begin();
#endif

#if Enable_TFT
    // 初始化TFT
    tft_begin();
#endif

    xTaskCreate(
        deviceTask,
        "Device Task",
        1024 * 10,
        NULL,
        1,
        NULL);

    xTaskCreate(
        btnTask,
        "Button Task",
        1024 * 10,
        NULL,
        1,
        NULL);

#if Enable_WIFI
    xTaskCreate(
        wifiTask,
        "WiFi Task",
        1024 * 10,
        NULL,
        1,
        NULL);
#endif

#if Enable_GPS
    xTaskCreate(
        gpsTask,
        "GPS Task",
        1024 * 10,
        NULL,
        1,
        &gpsTaskHandle);
#endif

    Serial.println("main setup end");
    device.print_device_info();
}

void loop()
{

#if Enable_BLE
    ble.loop();
#endif

#if Enable_WIFI
    mqtt.loop();
#endif

    // 获取GPS数据
#if Enable_GPS
    gps_data_t *gps_data = gps.get_gps_data();
    mqtt.publishGPS(*gps_data);
#endif

    // 获取IMU数据
#if Enable_IMU
    imu_data_t *imu_data = imu.get_imu_data(); // 使用getData()获取IMU数据结构
    mqtt.publishIMU(*imu_data);
#endif
    // 添加适当的延时
    delay(500); // 每秒发送一次数据
}