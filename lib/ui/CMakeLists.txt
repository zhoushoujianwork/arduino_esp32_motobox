SET(SOURCES screens/ui_Screen1.c
    ui.c
    components/ui_comp_hook.c
    ui_helpers.c
    images/ui_img_background_png.c
    images/ui_img_weixing_ok_png.c
    images/ui_img_bluetooth_1_png.c
    images/ui_img_wifiok_png.c
    images/ui_img_bat_png.c
    images/ui_img_guage_bg_png.c
    images/ui_img_speedcolor_png.c
    images/ui_img_speednu_png.c
    images/ui_img_h_png.c
    images/ui_img_gyro_png.c
    images/ui_img_gyrobackend_png.c
    images/ui_img_1142323638.c
    images/ui_img_frame_1_png.c
    images/ui_img_dir_img_30_png.c
    fonts/ui_font_Black_Ops_One.c
    fonts/ui_font_Black_Ops_One_14.c
    fonts/ui_font_Black_Ops_One_22.c)

add_library(ui ${SOURCES})
