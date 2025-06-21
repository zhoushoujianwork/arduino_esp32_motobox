#ifndef LED_TYPES_H
#define LED_TYPES_H

/**
 * @brief 统一的LED显示模式
 * 
 */
enum LEDMode
{
    LED_OFF,           // 关闭
    LED_ON,            // 常亮
    LED_BLINK_SINGLE,  // 单闪
    LED_BLINK_DUAL,    // 双闪
    LED_BLINK_SLOW,    // 慢闪
    LED_BLINK_FAST,    // 快闪
    LED_BREATH,        // 呼吸
    LED_BLINK_5_SECONDS // 5秒闪烁
};

/**
 * @brief 统一的LED颜色（仅PWM LED使用）
 * 
 */
enum LEDColor
{
    LED_COLOR_NONE,
    LED_COLOR_WHITE,  // GPS
    LED_COLOR_BLUE,   // WiFi
    LED_COLOR_YELLOW, // 4G
    LED_COLOR_PURPLE, // IMU
    LED_COLOR_GREEN,  // System OK
    LED_COLOR_RED     // System Error
};

#endif 