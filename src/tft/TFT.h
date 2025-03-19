#ifndef _TFT_H_
#define _TFT_H_

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <lvgl.h>
#include <ui.h>
#include "bat/BAT.h"
#include "config.h"
#include "gps/GPS.h"
#include "qmi8658/IMU.h"

void tft_begin();
void tft_loop();
void tft_sleep();
void tft_wakeup();
void tft_set_brightness(uint8_t brightness); // 0-255，0为最暗，255为最亮

// 声明为类成员变量或全局变量
extern float gyroTopLeft;
extern float gyroTopRight;
extern TFT_eSPI tft;

#endif // _TFT_H_