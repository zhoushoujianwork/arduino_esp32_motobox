/**
 * ESP32 MotoBox - 摩托车数据采集与显示系统
 * 
 * 硬件：ESP32-S3
 * 版本：2.0.0
 * 
 * 模式：
 * - MODE_ALLINONE: 独立模式，所有功能集成在一个设备上
 * - MODE_SERVER: 服务器模式，采集数据并通过BLE发送给客户端，同时通过MQTT发送到服务器
 * - MODE_CLIENT: 客户端模式，通过BLE接收服务器数据并显示
 * 
 * 编译选择：
 * - 使用PlatformIO环境配置: env:allinone, env:server, env:client
 */

#include "Arduino.h"
#include "config.h"
#include "ModuleManager.h"

/**
 * 主程序初始化
 */
void setup() {
    // 使用模块管理器初始化所有硬件模块
    moduleManager.begin();
    
    // 创建所有任务
    moduleManager.createTasks();
}

/**
 * 主循环
 * 留空，所有功能都在RTOS任务中处理
 */
void loop() {
    // 主循环留空，所有功能都在RTOS任务中处理
    delay(10000);
}