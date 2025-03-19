#include "TFT.h"

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[TFT_VER_RES * TFT_HOR_RES / 10];

TFT_eSPI tft = TFT_eSPI(); /* TFT instance */

float gyroTopLeft = 0;
float gyroTopRight = 0;
bool tft_is_waking_from_sleep = false; // 默认为false，表示不是从睡眠中唤醒

// 添加背光控制通道（假设使用PWM控制）
#ifdef TFT_BL
  static const int backlightChannel = 0; // 使用第一个PWM通道
  static const int backlightFreq = 5000; // 5KHz
  static const int backlightResolution = 8; // 8位分辨率 (0-255)
#endif

#if LV_USE_LOG != 0
/* Serial debugging */
void my_print(const char *buf)
{
    Serial.printf(buf);
    Serial.flush();
}
#endif

/* Display flushing */
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors((uint16_t *)&color_p->full, w * h, true);
    tft.endWrite();

    lv_disp_flush_ready(disp);
}

/*Read the touchpad*/
void my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data)
{
    uint16_t touchX = 0, touchY = 0;

    bool touched = false; // tft.getTouch( &touchX, &touchY, 600 );

    if (!touched)
    {
        data->state = LV_INDEV_STATE_REL;
    }
    else
    {
        data->state = LV_INDEV_STATE_PR;

        /*Set the coordinates*/
        data->point.x = touchX;
        data->point.y = touchY;

        Serial.print("Data x ");
        Serial.println(touchX);

        Serial.print("Data y ");
        Serial.println(touchY);
    }
}

void init_dashboard()
{
    for (int i = 0; i < UI_MAX_SPEED; i += 4)
    {
        lv_timer_handler(); // let the GUI do its work
        // delay(2);
        lv_slider_set_value(ui_SliderSpeed, i, LV_ANIM_OFF);
        lv_event_send(ui_SliderSpeed, LV_EVENT_VALUE_CHANGED, NULL);
        if (i <= UI_MAX_SPEED / 2)
        {
            lv_slider_set_value(ui_SliderGyro, map(i, 0, UI_MAX_SPEED / 2, 0, IMU_MAX_D), LV_ANIM_OFF);
        }
        else
        {
            lv_slider_set_value(ui_SliderGyro, map(i, UI_MAX_SPEED / 2, UI_MAX_SPEED, IMU_MAX_D, 0), LV_ANIM_OFF);
        }

        lv_event_send(ui_SliderGyro, LV_EVENT_VALUE_CHANGED, NULL);
    }
    for (int i = UI_MAX_SPEED; i >= 0; i -= 4)
    {
        lv_timer_handler(); // let the GUI do its work
        // delay(2);
        lv_slider_set_value(ui_SliderSpeed, i, LV_ANIM_OFF);
        lv_event_send(ui_SliderSpeed, LV_EVENT_VALUE_CHANGED, NULL);
        if (i >= UI_MAX_SPEED / 2)
        {
            lv_slider_set_value(ui_SliderGyro, map(i, UI_MAX_SPEED, UI_MAX_SPEED / 2, 0, -IMU_MAX_D), LV_ANIM_OFF);
        }
        else
        {
            lv_slider_set_value(ui_SliderGyro, map(i, UI_MAX_SPEED / 2, 0, -IMU_MAX_D, 0), LV_ANIM_OFF);
        }
        lv_event_send(ui_SliderGyro, LV_EVENT_VALUE_CHANGED, NULL);
    }

    // 最后速度设置为 0
    lv_slider_set_value(ui_SliderSpeed, 0, LV_ANIM_OFF);
    lv_event_send(ui_SliderSpeed, LV_EVENT_VALUE_CHANGED, NULL);
    lv_timer_handler();
}

void tft_begin()
{
    String LVGL_Arduino = "Hello Arduino! ";
    LVGL_Arduino += String('V') + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch();

    Serial.println(LVGL_Arduino);
    Serial.println("I am LVGL_Arduino");

    lv_init();

#if LV_USE_LOG != 0
    lv_log_register_print_cb(my_print); /* register print function for debugging */
#endif

    tft.begin();        /* TFT init */
    tft.setRotation(3); /* Landscape orientation, flipped */

    // 初始化背光
    #ifdef TFT_BL
    // 配置LEDC通道
    ledcSetup(backlightChannel, backlightFreq, backlightResolution);
    ledcAttachPin(TFT_BL, backlightChannel);
    // 设置默认亮度为最大
    ledcWrite(backlightChannel, 255);
    Serial.println("[TFT] 背光初始化完成，亮度设为最大");
    #endif

    lv_disp_draw_buf_init(&draw_buf, buf, NULL, TFT_VER_RES * TFT_HOR_RES / 10);

    /*Initialize the display*/
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    /*Change the following line to your display resolution*/
    disp_drv.hor_res = TFT_VER_RES;
    disp_drv.ver_res = TFT_HOR_RES;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    /*Initialize the (dummy) input device driver*/
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touchpad_read;
    lv_indev_drv_register(&indev_drv);

    ui_init();
    
    // 如果不是从睡眠唤醒，则执行初始化动画
    if (!tft_is_waking_from_sleep) {
        init_dashboard();
    } else {
        Serial.println("[TFT] 从睡眠模式唤醒，跳过初始化动画");
        // 重置唤醒标志
        tft_is_waking_from_sleep = false;
    }
    
    Serial.println("[TFT] 设置完成");
}

void tft_loop()
{

    lv_timer_handler(); // let the GUI do its work

    // Show battery level
    // 如果蓝牙没有连接则显示自己的电池电量！
    if (!device.get_device_state()->bleConnected)
    {
        lv_img_set_src(ui_imgBle, &ui_img_bluetooth_disconnect_png);
        lv_slider_set_value(ui_SliderBat, device.get_device_state()->battery_percentage, LV_ANIM_ON);
        lv_event_send(ui_SliderBat, LV_EVENT_VALUE_CHANGED, NULL);

        // 其他归零
        lv_obj_set_style_img_opa(ui_imgBle, LV_OPA_TRANSP, 0); // 完全透明
        lv_obj_set_style_img_opa(ui_imgWifi, LV_OPA_TRANSP, 0); // 完全透明
        lv_obj_set_style_img_opa(ui_imgGps, LV_OPA_TRANSP, 0); // 完全透明

        // 数据归零
        lv_label_set_text(ui_textGpsNu, "-");
        lv_label_set_text(ui_textGpsHz, "-");
        lv_label_set_text(ui_textTime, "1970-01-01 00:00:00");
        lv_label_set_text(ui_textLocation, "0.000000 / 0.000000");
        lv_label_set_text(ui_textTrip, "Trip:--- km");
        lv_slider_set_value(ui_SliderSpeed, 0, LV_ANIM_OFF);
        lv_event_send(ui_SliderSpeed, LV_EVENT_VALUE_CHANGED, NULL);
        lv_slider_set_value(ui_SliderGyro, 0, LV_ANIM_OFF);
        lv_event_send(ui_SliderGyro, LV_EVENT_VALUE_CHANGED, NULL);
    }
    else
    {
        lv_img_set_src(ui_imgBle, &ui_img_bluetooth_1_png);
        lv_obj_set_style_img_opa(ui_imgBle, LV_OPA_COVER, 0); // 完全不透明
        lv_obj_set_style_img_opa(ui_imgWifi, LV_OPA_COVER, 0); // 完全不透明
        lv_obj_set_style_img_opa(ui_imgGps, LV_OPA_COVER, 0); // 完全不透明

        // Show Wifi
        if (device.get_device_state()->wifiConnected)
        {
            lv_img_set_src(ui_imgWifi, &ui_img_wifiok_png);
        }
        else
        {
            lv_img_set_src(ui_imgWifi, &ui_img_wificon_png);
        }

        

        // Show Gps
        char gpsTextnu[4]; // 增加缓冲区大小以容纳两位数字和结束符
        snprintf(gpsTextnu, sizeof(gpsTextnu), "%d", device.get_gps_data()->satellites);
        lv_label_set_text(ui_textGpsNu, gpsTextnu);

        char gpsTextHz[4]; // 增加缓冲区大小以容纳两位数字和结束符
        snprintf(gpsTextHz, sizeof(gpsTextHz), "%d", device.get_device_state()->gpsHz);
        lv_label_set_text(ui_textGpsHz, gpsTextHz);

        char gpsTextTime[20]; // 2024-09-05 10:00:00
        snprintf(gpsTextTime, sizeof(gpsTextTime), "%04d-%02d-%02d %02d:%02d:%02d", device.get_gps_data()->year,
                 device.get_gps_data()->month, device.get_gps_data()->day, device.get_gps_data()->hour,
                 device.get_gps_data()->minute, device.get_gps_data()->second);
        lv_label_set_text(ui_textTime, gpsTextTime);
        char gpsTextLocation[20]; // 120'123456 / 21'654321
        snprintf(gpsTextLocation, sizeof(gpsTextLocation), "%s / %s", String(device.get_gps_data()->latitude, 6), String(device.get_gps_data()->longitude, 6));
        lv_label_set_text(ui_textLocation, gpsTextLocation);

        // 处理GPS无数据的情况
        int currentSpeedValue = device.get_gps_data()->speed;
        if (device.get_device_state()->gpsReady)
        {
            lv_obj_set_style_img_opa(ui_imgGps, LV_OPA_COVER, 0); // 完全不透明
        }
        else
        {
            currentSpeedValue = 0;
            lv_obj_set_style_img_opa(ui_imgGps, LV_OPA_TRANSP, 0); // 完全透明
        }
        lv_slider_set_value(ui_SliderSpeed, currentSpeedValue, LV_ANIM_OFF);
        lv_event_send(ui_SliderSpeed, LV_EVENT_VALUE_CHANGED, NULL);
        char tripText[20];                             // Ensure this buffer is large enough for your number
        snprintf(tripText, sizeof(tripText), "Trip:%.0f km", device.getTotalDistanceKm());
        lv_label_set_text(ui_textTrip, tripText);

        // 依据方向移动Compose TODO: 优化这块图片准确性
        lv_obj_set_x(ui_compass, map(device.get_gps_data()->heading, 0, 360, 150, -195));

        lv_slider_set_value(ui_SliderBat, device.get_device_state()->battery_percentage, LV_ANIM_ON);
        lv_event_send(ui_SliderBat, LV_EVENT_VALUE_CHANGED, NULL);

        // Show Gyro
        float gyro = device.get_imu_data()->pitch;
        // float gyro = imu.get_imu_data()->roll;
        lv_slider_set_value(ui_SliderGyro, gyro, LV_ANIM_ON);
        lv_event_send(ui_SliderGyro, LV_EVENT_VALUE_CHANGED, NULL);
        // top值只保留 2 秒
        // unsigned long currentMillis = millis();
        // if (currentMillis - previousMillis >= 2000)
        // {
        //     previousMillis = currentMillis;
        //     gyroTopLeft = 0;
        //     gyroTopRight = 0;
        // }

        // get top gyro value for show
        if (gyro > gyroTopRight)
        {
            gyroTopRight = gyro;
            if (gyro > IMU_MAX_D)
                gyroTopRight = IMU_MAX_D;
        }
        if (gyro < gyroTopLeft)
        {
            gyroTopLeft = gyro;
            if (gyro < -IMU_MAX_D)
                gyroTopLeft = -IMU_MAX_D;
        }
        char gyroTextLeft[20];
        char gyroTextRight[20];
        snprintf(gyroTextLeft, sizeof(gyroTextLeft), "%.0f°", gyroTopLeft);
        snprintf(gyroTextRight, sizeof(gyroTextRight), "%.0f°", gyroTopRight);
        lv_label_set_text(ui_textGyroTopLeft, gyroTextLeft);
        lv_label_set_text(ui_textGyroTopRight, gyroTextRight);

        // 添加HDOP状态显示
        // char hdopText[20];
        // float hdop = device.get_gps_data()->hdop;
        // const char* status = "";
        // if(hdop < 1.0) status = "优秀";
        // else if(hdop < 2.0) status = "良好";
        // else if(hdop < 5.0) status = "一般";
        // else status = "较差";
        
        // snprintf(hdopText, sizeof(hdopText), "精度:%.1f(%s)", hdop, status);
        // lv_label_set_text(ui_textHdopStatus, hdopText);
    }
}

void tft_sleep() {
    Serial.println("[TFT] 进入睡眠模式");
    
    // 首先将亮度逐渐降低到0
    #ifdef TFT_BL
    // 以10步逐渐降低亮度
    for (int brightness = 255; brightness >= 0; brightness -= 25) {
        tft_set_brightness(brightness);
        delay(20); // 短暂延迟以实现渐变效果
    }
    // 最后确保亮度为0
    tft_set_brightness(0);
    #endif
    
    // 发送指令使显示器进入睡眠模式
    tft.writecommand(0x10); // 发送睡眠命令
}

void tft_wakeup() {
    Serial.println("[TFT] 唤醒显示屏");
    
    // 设置唤醒标志，告知tft_begin跳过初始化动画
    tft_is_waking_from_sleep = true;
    
    // 发送指令唤醒显示器
    tft.writecommand(0x11); // 发送唤醒命令
    delay(120); // ST7789等显示器需要等待一段时间
    
    // 逐渐增加亮度
    #ifdef TFT_BL
    // 确保亮度初始为0
    tft_set_brightness(0);
    // 以10步逐渐提高亮度
    for (int brightness = 0; brightness <= 255; brightness += 25) {
        tft_set_brightness(brightness);
        delay(20); // 短暂延迟以实现渐变效果
    }
    // 最后确保亮度为最大
    tft_set_brightness(255);
    #endif
    
    // 刷新LVGL界面，确保显示正常
    lv_timer_handler();
    Serial.println("[TFT] 显示屏唤醒完成");
}

void tft_set_brightness(uint8_t brightness) {
  #ifdef TFT_BL
    // 使用ledc设置PWM值控制亮度
    if (brightness > 255) brightness = 255;
    
    // 检查是否已经初始化过PWM
    static bool pwm_initialized = false;
    if (!pwm_initialized) {
      // 配置LEDC通道
      ledcSetup(backlightChannel, backlightFreq, backlightResolution);
      ledcAttachPin(TFT_BL, backlightChannel);
      pwm_initialized = true;
    }
    
    // 设置亮度
    ledcWrite(backlightChannel, brightness);
    Serial.printf("[TFT] 设置亮度为 %d/255\n", brightness);
  #else
    Serial.println("[TFT] 未定义TFT_BL引脚，无法调整亮度");
  #endif
}
