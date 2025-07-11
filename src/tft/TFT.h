#ifndef _TFT_H_
#define _TFT_H_

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <lvgl.h>
#include <ui.h>
#include "bat/BAT.h"
#include "config.h"
#include "imu/qmi8658.h"
#include "device.h"

void tft_begin();
void tft_loop();
void tft_sleep();
void tft_wakeup();
void tft_set_brightness(uint8_t brightness); // 0-255，0为最暗，255为最亮
void init_dashboard(); // 仪表盘初始化动画
void tft_show_notification(const char *title, const char *message, uint32_t duration); // 显示通知

// 声明为类成员变量或全局变量
extern float gyroTopLeft;
extern float gyroTopRight;
extern TFT_eSPI tft;
extern bool tft_is_waking_from_sleep; // 标记TFT是否正从睡眠中唤醒

#endif // _TFT_H_