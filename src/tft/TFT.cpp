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
    for (int i = 0; i < UI_MAX_SPEED; i += 10)
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
    for (int i = UI_MAX_SPEED; i >= 0; i -= 10)
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
    Serial.println("[TFT] 初始化LVGL");

    // 检查是否已经从睡眠中唤醒
    bool isWakingFromSleep = tft_is_waking_from_sleep;
    
    // 初始化LVGL
    lv_init();

#if LV_USE_LOG != 0
    lv_log_register_print_cb(my_print); /* register print function for debugging */
#endif

    // 初始化显示屏
    tft.begin(); /* TFT init */
    tft.setRotation(TFT_ROTATION); /* Landscape orientation, flipped */

    // 初始化背光
    #ifdef TFT_BL
    // 配置LEDC通道
    ledcSetup(backlightChannel, backlightFreq, backlightResolution);
    ledcAttachPin(TFT_BL, backlightChannel);
    // 设置默认亮度为最大
    ledcWrite(backlightChannel, 250);
    Serial.println("[TFT] 背光初始化完成，亮度设为最大");
    #endif

    // 如果是从睡眠中唤醒，执行唤醒命令
    if (isWakingFromSleep) {
        Serial.println("[TFT] 从睡眠中唤醒，发送唤醒命令");
        tft.writecommand(0x11); // SLPOUT - 退出睡眠模式
        delay(120); // 等待唤醒
        tft.writecommand(0x29); // DISPON - 打开显示
        delay(50);
    }

    // 初始化LVGL绘图缓冲区
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

    // 初始化UI
    ui_init();
    
    // 如果不是从睡眠唤醒，则执行初始化动画
    if (!isWakingFromSleep) {
        Serial.println("[TFT] 执行仪表盘初始化动画");
        init_dashboard();
    } else {
        Serial.println("[TFT] 从睡眠模式唤醒，跳过初始化动画");
        // 重置仪表为零值
        lv_slider_set_value(ui_SliderSpeed, 0, LV_ANIM_OFF);
        lv_event_send(ui_SliderSpeed, LV_EVENT_VALUE_CHANGED, NULL);
        lv_slider_set_value(ui_SliderGyro, 0, LV_ANIM_OFF);
        lv_event_send(ui_SliderGyro, LV_EVENT_VALUE_CHANGED, NULL);
    }
    
    // 重置唤醒标志
    tft_is_waking_from_sleep = false;
    
    // 确保界面刷新
    lv_timer_handler();
    
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
        // 显示蓝牙图标 - 已连接状态
        lv_img_set_src(ui_imgBle, &ui_img_bluetooth_1_png);
        lv_obj_set_style_img_opa(ui_imgBle, LV_OPA_COVER, 0); // 完全不透明
        
        // Wi-Fi图标始终显示，但根据连接状态调整透明度
        lv_obj_set_style_img_opa(ui_imgWifi, LV_OPA_COVER, 0); // 默认完全不透明
        
        // GPS图标根据数据有效性显示
        lv_obj_set_style_img_opa(ui_imgGps, device.get_device_state()->gpsReady ? LV_OPA_COVER : LV_OPA_TRANSP, 0);

        // 根据WiFi连接状态更新图标
        if (device.get_device_state()->wifiConnected) {
            // WiFi已连接 - 显示正常图标但完全不透明
            lv_img_set_src(ui_imgWifi, &ui_img_wificon_png);
            lv_obj_set_style_img_opa(ui_imgWifi, LV_OPA_COVER, 0); // 完全不透明
        } else {
            // WiFi未连接 - 使用相同图标但半透明
            lv_img_set_src(ui_imgWifi, &ui_img_wificon_png);
            lv_obj_set_style_img_opa(ui_imgWifi, LV_OPA_50, 0); // 半透明
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
        if (!device.get_device_state()->gpsReady) {
            currentSpeedValue = 0;
        }
        
        lv_slider_set_value(ui_SliderSpeed, currentSpeedValue, LV_ANIM_OFF);
        lv_event_send(ui_SliderSpeed, LV_EVENT_VALUE_CHANGED, NULL);
        char tripText[20];                             // Ensure this buffer is large enough for your number
        snprintf(tripText, sizeof(tripText), "Trip:%.0f km", device.getTotalDistanceKm());
        lv_label_set_text(ui_textTrip, tripText);

        // 依据方向移动Compass
        lv_obj_set_x(ui_compass, device.get_compass_data()->heading);

        lv_slider_set_value(ui_SliderBat, device.get_device_state()->battery_percentage, LV_ANIM_ON);
        lv_event_send(ui_SliderBat, LV_EVENT_VALUE_CHANGED, NULL);

        // Show Gyro
        float gyro = device.get_imu_data()->pitch;
        // float gyro = imu.get_imu_data()->roll;
        lv_slider_set_value(ui_SliderGyro, gyro, LV_ANIM_ON);
        lv_event_send(ui_SliderGyro, LV_EVENT_VALUE_CHANGED, NULL);

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
    }
}

void tft_sleep() {
    Serial.println("[TFT] 进入睡眠模式");
    
    // 首先将亮度逐渐降低到0
    #ifdef TFT_BL
    // 以10步逐渐降低亮度
    for (int brightness = 255; brightness >= 0; brightness -= 2) {
        tft_set_brightness(brightness);
        delay(2); // 短暂延迟以实现渐变效果
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
    
    // 设置MADCTL寄存器，修复颜色和方向问题
    tft.writecommand(TFT_MADCTL);
    // 反转颜色顺序：从BGR改为RGB或从RGB改为BGR
    tft.writedata(TFT_MAD_MX | TFT_MAD_MY | TFT_MAD_RGB); // 使用RGB顺序
    delay(10);
    
    // 打开显示
    tft.writecommand(0x29); // DISPON - 打开显示
    delay(50);
    
    // 设置屏幕旋转方向
    tft.setRotation(3); // Landscape orientation, flipped
    Serial.println("[TFT] 重新设置屏幕方向为横屏模式");
    
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

void tft_show_notification(const char *title, const char *message, uint32_t duration) {
    #if defined(MODE_ALLINONE) || defined(MODE_CLIENT)
    // 创建通知对话框
    static lv_obj_t *notification = NULL;
    
    // 如果已经有通知，先删除旧的
    if (notification != NULL) {
        lv_obj_del(notification);
        notification = NULL;
    }
    
    // 创建新通知对话框
    notification = lv_obj_create(lv_scr_act());
    
    // 设置通知样式
    lv_obj_set_size(notification, 150, 80);
    lv_obj_align(notification, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_set_style_bg_color(notification, lv_color_hex(0x2196F3), LV_PART_MAIN); // 蓝色背景
    lv_obj_set_style_radius(notification, 10, LV_PART_MAIN); // 圆角
    lv_obj_set_style_shadow_width(notification, 10, LV_PART_MAIN); // 添加阴影
    lv_obj_set_style_shadow_color(notification, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(notification, 50, LV_PART_MAIN);
    
    // 创建标题
    lv_obj_t *title_label = lv_label_create(notification);
    lv_label_set_text(title_label, title);
    lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 5);
    lv_obj_set_style_text_color(title_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN); // 白色文字
    
    // 创建内容
    lv_obj_t *msg_label = lv_label_create(notification);
    lv_label_set_text(msg_label, message);
    lv_obj_align(msg_label, LV_ALIGN_CENTER, 0, 10);
    lv_obj_set_style_text_color(msg_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN); // 白色文字
    
    // 设置动画效果
    lv_obj_fade_in(notification, 300, 0);
    
    // 创建定时器自动关闭通知
    static lv_timer_t *close_timer = NULL;
    if (close_timer != NULL) {
        lv_timer_del(close_timer);
    }
    
    // 设置通知自动关闭
    close_timer = lv_timer_create([](lv_timer_t *timer) {
        lv_obj_t *notif = (lv_obj_t*)timer->user_data;
        lv_obj_fade_out(notif, 300, 0);
        lv_timer_t *close_timer = lv_timer_create([](lv_timer_t *timer) {
            lv_obj_t *notif = (lv_obj_t*)timer->user_data;
            lv_obj_del(notif);
            lv_timer_del(timer);
        }, 300, notif);
        lv_timer_set_repeat_count(close_timer, 1);
        lv_timer_del(timer);
    }, duration, notification);
    
    lv_timer_set_repeat_count(close_timer, 1);
    
    // 刷新显示
    lv_timer_handler();
    #endif
}
