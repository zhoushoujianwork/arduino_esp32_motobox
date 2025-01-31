#ifndef _TFT_H_
#define _TFT_H_

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <lvgl.h>
#include <ui.h>
#include "config.h"
#include "gps/GPS.h"
#include "qmi8658/IMU.h"

class TFT
{
private:
    TFT_eSPI *tft;
    lv_disp_draw_buf_t draw_buf;
    lv_color_t *buf;
    lv_disp_drv_t disp_drv;
    uint8_t rotation;
    bool initialized;

    // 添加静态成员函数作为回调
    static void disp_flush_cb(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p);
    static void touchpad_read_cb(lv_indev_drv_t *indev_driver, lv_indev_data_t *data);

    // 保持非静态成员函数用于实际实现
    void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p);
    void my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data);

    // 添加静态指针指向当前实例
    static TFT *instance;

public:
    TFT(uint8_t rotation);

    static TFT *getInstance();
    static void releaseInstance();

    float gyroTopRight;
    float gyroTopLeft;
    void begin();
    void loop();
};

#endif // _TFT_H_