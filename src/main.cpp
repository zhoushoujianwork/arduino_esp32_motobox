#include <Arduino.h>
#include "btn/BTN.h"
#include "led/LED.h"
#include "bat/BAT.h"
#include "device.h"
#include "config.h"
#include "tft/TFT.h"
// 定义任务句柄
TaskHandle_t deviceTaskHandle = NULL;
TaskHandle_t btnTaskHandle = NULL;
TaskHandle_t gpsTaskHandle = NULL;

Device device;
#include "ble/ble.h"
#if Enable_BLE
BLE ble;
#endif
#if Enable_IMU
#include "QMI8658/imu.h"
IMU imu(42, 2);
#endif
#if Enable_GPS
#include "gps/GPS.h"
GPS gps(12, 11);
#endif

BAT bat(7, 2900, 3300); // 电池电压 3.3V - 4.2V
BTN button(39);
LED led(8); // 假设LED连接在GPIO 8上

TFT tft(3); // 创建TFT对象，rotation设为3

void deviceTask(void *parameter)
{
    while (true)
    {
        bat.loop();

#if Enable_BLE
        ble.loop();
#endif
        // bat.print_voltage();
        device.set_battery(bat.getPercentage());
        delay(2000);
    }
}

// 按钮任务
void btnTask(void *parameter)
{
    static int hzs[] = {1, 2, 5, 10}; // 支持的更新率
    static int hz = 0;                // 当前更新率索引

    while (true)
    {
        if (button.isClicked())
        {
            // 循环切换更新率
            hz = (hz + 1) % 4;
#if Enable_GPS
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

        // 更新TFT
#if Enable_TFT
        tft.loop();
#endif

        delay(10);
    }
}

// gps无时间延迟的任务
void gpsTask(void *parameter)
{
    while (true)
    {
#if Enable_GPS
        gps.loop();
        // gps.printRawData();
#endif
    }
}

void setup()
{
    Serial.begin(115200);

#if Enable_TFT
    // 初始化TFT
    tft.begin();
#endif

#if Enable_IMU
    // 添加IMU初始化检查
    imu.begin();
#endif

#if Enable_GPS
    gps.begin();
#endif
    led.begin();

    led.setMode(LED::OFF);
    delay(1000);

    xTaskCreatePinnedToCore(
        deviceTask,
        "Device Task",
        4000,
        NULL,
        1,
        &deviceTaskHandle,
        0);

    xTaskCreatePinnedToCore(
        btnTask,
        "Button Task",
        4000,
        NULL,
        1,
        &btnTaskHandle,
        1);

#if Enable_GPS
    xTaskCreatePinnedToCore(
        gpsTask,
        "GPS Task",
        4000,
        NULL,
        1,
        &gpsTaskHandle,
        1);
#endif

    device.print_device_info();

#if Enable_BLE
    ble.begin();
#endif

    delay(1000);
    Serial.println("setup end");
}

void loop()
{
    led.loop();

#if Enable_IMU
    // imu.printImuData();
    imu.loop();
#endif

#if Enable_GPS
    gps.printGpsData();
#endif

    // device.print_device_info();
    if (device.get_ble_connected())
    {
        led.setMode(LED::BLINK_DUAL);
    }
    else
    {
        led.setMode(LED::OFF);
    }
    delay(200);
}