#include "utils/serialCommand.h"

// ===================== 串口命令处理函数 =====================
/**
 * 处理串口输入命令
 */
void handleSerialCommand()
{
    String command = Serial.readStringUntil('\n');
    command.trim();

    if (command.length() > 0)
    {
        Serial.println(">>> 收到命令: " + command);

#ifdef ENABLE_SDCARD
        // SD卡相关命令
        if (command.startsWith("sd."))
        {
            sdManager.handleSerialCommand(command);
        }
#endif
        if (command == "info")
        {
            Serial.println("=== 设备信息 ===");
            Serial.println("设备ID: " + device_state.device_id);
            Serial.println("固件版本: " + String(device_state.device_firmware_version));
            Serial.println("硬件版本: " + String(device_state.device_hardware_version));
            Serial.println("运行时间: " + String(millis() / 1000) + " 秒");
            Serial.println("");
            Serial.println("--- 连接状态 ---");
            Serial.println("WiFi状态: " + String(device_state.wifiConnected ? "已连接" : "未连接"));
            Serial.println("BLE状态: " + String(device_state.bleConnected ? "已连接" : "未连接"));
#ifdef ENABLE_GSM
            Serial.println("GSM状态: " + String(device_state.gsmReady ? "就绪" : "未就绪"));
#ifdef USE_AIR780EG_GSM
            if (device_state.gsmReady)
            {
                Serial.println("网络状态: " + String(air780eg.getNetwork().isNetworkRegistered() ? "已连接" : "未连接"));
                Serial.println("信号强度: " + String(air780eg.getNetwork().getSignalStrength()) + " dBm");
                Serial.println("运营商: " + air780eg.getNetwork().getOperatorName());
                if (air780eg.getGNSS().isFixed())
                {
                    Serial.println("GNSS状态: 已定位 (卫星数: " + String(air780eg.getGNSS().getSatelliteCount()) + ")");
                }
                else
                {
                    Serial.println("GNSS状态: 未定位 (卫星数: " + String(air780eg.getGNSS().getSatelliteCount()) + ")");
                }
            }
#endif

            // MQTT连接状态和配置信息
#ifndef DISABLE_MQTT
            Serial.println("MQTT状态: " + air780eg.getMQTT().getState());
            Serial.println("MQTT服务器: " + String(MQTT_BROKER) + ":" + String(MQTT_PORT));

            // 显示已注册的主题
            Serial.println("--- MQTT主题配置 ---");
            String deviceId = device_state.device_id;
            String baseTopic = "vehicle/v1/" + deviceId;
            Serial.println("基础主题: " + baseTopic);
            Serial.println("设备信息: " + baseTopic + "/telemetry/device");
            Serial.println("位置信息: " + baseTopic + "/telemetry/location");
            Serial.println("运动信息: " + baseTopic + "/telemetry/motion");
            Serial.println("控制命令: " + baseTopic + "/ctrl/#");
#else
            Serial.println("MQTT功能: ❌ 已禁用");
#endif
#endif
            Serial.println("");
            Serial.println("--- 传感器状态 ---");
            Serial.println("GNSS状态: " + String(device_state.gnssReady ? "就绪" : "未就绪"));
            Serial.println("IMU状态: " + String(device_state.imuReady ? "就绪" : "未就绪"));
            Serial.println("罗盘状态: " + String(device_state.compassReady ? "就绪" : "未就绪"));
            Serial.println("");
            Serial.println("--- 音频设备状态 ---");
#ifdef ENABLE_AUDIO
            Serial.println("音频系统: " + String(device_state.audioReady ? "✅ 就绪" : "❌ 未就绪"));
            if (device_state.audioReady)
            {
                Serial.printf("音频引脚: WS=%d, BCLK=%d, DATA=%d\n", IIS_S_WS_PIN, IIS_S_BCLK_PIN, IIS_S_DATA_PIN);
                Serial.println("音频芯片: NS4168 功率放大器");
                Serial.println("采样率: 16kHz");
                Serial.println("位深度: 16位");
                Serial.println("声道: 单声道");
                Serial.println("支持事件: 开机音/WiFi连接音/GPS定位音/低电量音/睡眠音");
            }
            else
            {
                Serial.println("⚠️ 音频系统未就绪，请检查:");
                Serial.println("  1. I2S引脚连接是否正确");
                Serial.println("  2. NS4168芯片是否正常工作");
                Serial.println("  3. 音频引脚是否与其他功能冲突");
            }
#else
            Serial.println("音频系统: ❌ 未启用 (编译时禁用)");
#endif
            Serial.println("");
            Serial.println("--- 电源状态 ---");
            Serial.println("电池电压: " + String(device_state.battery_voltage) + " mV");
            Serial.println("电池电量: " + String(device_state.battery_percentage) + "%");
            Serial.println("充电状态: " + String(device_state.is_charging ? "充电中" : "未充电"));
            Serial.println("外部电源: " + String(device_state.external_power ? "已连接" : "未连接"));
            Serial.println("");
#ifdef ENABLE_SDCARD
            Serial.println("--- SD卡状态 ---");
            if (device_state.sdCardReady)
            {
                Serial.println("SD卡状态: 就绪");
                Serial.println("SD卡容量: " + String((unsigned long)device_state.sdCardSizeMB) + " MB");
                Serial.println("SD卡剩余: " + String((unsigned long)device_state.sdCardFreeMB) + " MB");
            }
            else
            {
                Serial.println("SD卡状态: 未就绪");
                Serial.println("⚠️ 请检查SD卡是否正确插入");
            }
#endif
        }
        else if (command == "status")
        {
            Serial.println("=== 系统状态 ===");
            Serial.println("系统正常运行");
            Serial.println("空闲内存: " + String(ESP.getFreeHeap()) + " 字节");
            Serial.println("最小空闲内存: " + String(ESP.getMinFreeHeap()) + " 字节");
            Serial.println("芯片温度: " + String(temperatureRead(), 1) + "°C");
            Serial.println("CPU频率: " + String(ESP.getCpuFreqMHz()) + " MHz");
        }
        else if (command.startsWith("mqtt."))
        {
#ifndef DISABLE_MQTT
            if (command == "mqtt.status")
            {
                Serial.println("=== MQTT状态 ===");
                Serial.println("MQTT服务器: " + String(MQTT_BROKER));
                Serial.println("MQTT端口: " + String(MQTT_PORT));
                Serial.println("保持连接: " + String(MQTT_KEEPALIVE) + "秒");

#ifdef USE_AIR780EG_GSM
                Serial.println("连接方式: Air780EG GSM");
                Serial.println("GSM状态: " + String(device_state.gsmReady ? "就绪" : "未就绪"));
                if (device_state.gsmReady)
                {
                    Serial.println("网络状态: " + String(air780eg.getNetwork().isNetworkRegistered() ? "已连接" : "未连接"));
                    Serial.println("信号强度: " + String(air780eg.getNetwork().getSignalStrength()) + " dBm");
                    Serial.println("运营商: " + air780eg.getNetwork().getOperatorName());
                }
#elif defined(ENABLE_WIFI)
                Serial.println("连接方式: WiFi");
                Serial.println("WiFi状态: " + String(device_state.wifiConnected ? "已连接" : "未连接"));
#endif
                // MQTT连接状态
                Serial.println("MQTT连接: " + air780eg.getMQTT().getState());
            }
            else
            {
                Serial.println("未知MQTT命令，输入 'mqtt.help' 查看帮助");
            }
#else
            Serial.println("MQTT功能已禁用");
#endif
        }
        else if (command.startsWith("audio."))
        {
#ifdef ENABLE_AUDIO
            if (command == "audio.test")
            {
                Serial.println("=== 音频系统测试 ===");
                if (device_state.audioReady)
                {
                    Serial.println("播放测试音频序列...");
                    audioManager.testAudio();
                }
                else
                {
                    Serial.println("❌ 音频系统未就绪，无法测试");
                }
            }
            else if (command == "audio.boot")
            {
                Serial.println("播放开机成功音...");
                if (device_state.audioReady)
                {
                    audioManager.playBootSuccessSound();
                }
                else
                {
                    Serial.println("❌ 音频系统未就绪");
                }
            }
            else if (command == "audio.wifi")
            {
                Serial.println("播放WiFi连接音...");
                if (device_state.audioReady)
                {
                    audioManager.playWiFiConnectedSound();
                }
                else
                {
                    Serial.println("❌ 音频系统未就绪");
                }
            }
            else if (command == "audio.gps")
            {
                Serial.println("播放GPS定位音...");
                if (device_state.audioReady)
                {
                    audioManager.playGPSFixedSound();
                }
                else
                {
                    Serial.println("❌ 音频系统未就绪");
                }
            }
            else if (command == "audio.battery")
            {
                Serial.println("播放低电量音...");
                if (device_state.audioReady)
                {
                    audioManager.playLowBatterySound();
                }
                else
                {
                    Serial.println("❌ 音频系统未就绪");
                }
            }
            else if (command == "audio.sleep")
            {
                Serial.println("播放睡眠音...");
                if (device_state.audioReady)
                {
                    audioManager.playSleepModeSound();
                }
                else
                {
                    Serial.println("❌ 音频系统未就绪");
                }
            }
            else if (command == "audio.help")
            {
                Serial.println("=== 音频命令帮助 ===");
                Serial.println("audio.test    - 播放测试音频序列");
                Serial.println("audio.boot    - 播放开机成功音");
                Serial.println("audio.wifi    - 播放WiFi连接音");
                Serial.println("audio.gps     - 播放GPS定位音");
                Serial.println("audio.battery - 播放低电量音");
                Serial.println("audio.sleep   - 播放睡眠音");
                Serial.println("audio.help    - 显示此帮助信息");
            }
            else
            {
                Serial.println("未知音频命令，输入 'audio.help' 查看帮助");
            }
#else
            Serial.println("音频功能未启用");
#endif
        }
        else if (command == "restart" || command == "reboot")
        {
            Serial.println("正在重启设备...");
            Serial.flush();
            delay(1000);
            ESP.restart();
        }
        else if (command == "help")
        {
            Serial.println("=== 可用命令 ===");
            Serial.println("基本命令:");
            Serial.println("  info     - 显示详细设备信息");
            Serial.println("  status   - 显示系统状态");
            Serial.println("  restart  - 重启设备");
            Serial.println("  help     - 显示此帮助信息");
            Serial.println("");
#ifdef ENABLE_SDCARD
            Serial.println("SD卡命令:");
            Serial.println("  sd.info    - 显示SD卡详细信息");
            Serial.println("  sd.test    - 测试GPS数据记录");
            Serial.println("  sd.status  - 检查SD卡状态");
            Serial.println("  sd.session - 显示当前GPS会话信息");
            Serial.println("  sd.finish  - 结束当前GPS会话");
            Serial.println("  sd.dirs    - 检查和创建目录结构");
            Serial.println("");
#endif
#ifdef USE_AIR780EG_GSM
            Serial.println("Air780EG命令:");
            Serial.println("  gsm.test   - 测试AT命令和波特率");
            Serial.println("  gsm.reset  - 重置Air780EG模块");
            Serial.println("  gsm.info   - 显示模块状态信息");
            Serial.println("  gsm.mqtt   - 测试MQTT功能支持");
            Serial.println("  gsm.mqtt.debug - MQTT连接详细调试");
            Serial.println("  gsm.lbs    - LBS基站定位调试");
            Serial.println("  gsm.lbs.test - LBS AT命令测试");
            Serial.println("  gps.status - SimpleGPS 状态查询");
            Serial.println("");
#endif
            Serial.println("提示: 命令不区分大小写");
        }
        else
        {
            Serial.println("❌ 未知命令: " + command);
            Serial.println("输入 'help' 查看可用命令");
        }

        Serial.println(""); // 添加空行分隔
    }
}
