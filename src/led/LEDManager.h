#ifndef LED_MANAGER_H
#define LED_MANAGER_H

#include "PWMLED.h"
#include "../device.h"

class LEDManager {
public:
    // LED状态类型
    enum StateType {
        NETWORK_STATE,    // 网络状态
        GPS_STATE,        // GPS状态
        SYSTEM_STATE     // 系统状态
    };

    // 具体状态定义
    struct LEDState {
        PWMLED::Mode mode;          // LED模式（常亮/呼吸/闪烁）
        PWMLED::ModuleColor color;  // LED颜色
        uint8_t brightness;         // 亮度
    };

    static LEDManager& getInstance() {
        static LEDManager instance;
        return instance;
    }

    void begin() {
#ifdef PWM_LED_PIN
        pwmLed.begin();
#endif
    }

    void loop() {
#ifdef PWM_LED_PIN
        // 获取当前设备状态
        const device_state_t& state = *get_device_state();
        
        // 检查是否所有模块都正常
        bool allModulesReady = state.gpsReady && 
                              state.wifiConnected && 
                              state.imuReady;

        if (allModulesReady) {
            setLEDState(PWMLED::BREATH, PWMLED::GREEN);
        }else if (state.wifiConnected) {
            setLEDState(PWMLED::SOLID, PWMLED::BLUE);
        }else if (state.gpsReady) {
            setLEDState(PWMLED::SOLID, PWMLED::WHITE);
        }else if (state.imuReady) {
            setLEDState(PWMLED::SOLID, PWMLED::PURPLE);
        }
        
        // // 初始化阶段，白色慢闪
        // else if (!state.initialized) {
        //     setLEDState(PWMLED::BLINK_SLOW, PWMLED::WHITE);
        // }
        // 网络连接中
        // else if (state.wifiConnecting) {
        //     setLEDState(PWMLED::BLINK_FAST, PWMLED::BLUE);
        // }
        // // GPS 搜星中
        // else if (state.gpsSearching) {
        //     setLEDState(PWMLED::BREATH, PWMLED::WHITE);
        // }
        // // 4G 连接中
        // else if (state.gsmConnecting) {
        //     setLEDState(PWMLED::BLINK_FAST, PWMLED::YELLOW);
        // }
        // // IMU 校准中
        // else if (state.imuCalibrating) {
        //     setLEDState(PWMLED::BREATH, PWMLED::PURPLE);
        // }
        // 模块状态检查
        // else {
        //     // GPS 未就绪时优先显示 GPS 状态
        //     if (!state.gpsReady) {
        //         setLEDState(PWMLED::BLINK_SLOW, PWMLED::WHITE);
        //     }
        //     // 其次是网络状态
        //     else if (!state.wifiConnected && !state.gsmReady) {
        //         setLEDState(PWMLED::BLINK_SLOW, PWMLED::BLUE);
        //     }
        //     // 最后是 IMU 状态
        //     else if (!state.imuReady) {
        //         setLEDState(PWMLED::BLINK_SLOW, PWMLED::PURPLE);
        //     }
        // }

        // 运行 LED 效果
        pwmLed.loop();
#endif
    }

    // 设置网络状态
    void setNetworkState(PWMLED::Mode mode, PWMLED::ModuleColor color, uint8_t brightness = 10) {
        if (_currentType != NETWORK_STATE || 
            _currentState.mode != mode || 
            _currentState.color != color) {
            
            _currentType = NETWORK_STATE;
            _currentState = {mode, color, brightness};
            updateLED();
        }
    }

    // 设置GPS状态
    void setGPSState(PWMLED::Mode mode, PWMLED::ModuleColor color, uint8_t brightness = 10) {
        // 只有当没有网络状态显示时才显示GPS状态
        if (_currentType != NETWORK_STATE) {
            if (_currentType != GPS_STATE || 
                _currentState.mode != mode || 
                _currentState.color != color) {
                
                _currentType = GPS_STATE;
                _currentState = {mode, color, brightness};
                updateLED();
            }
        }
    }

    // 设置系统状态
    void setSystemState(PWMLED::Mode mode, PWMLED::ModuleColor color, uint8_t brightness = 10) {
        // 系统状态优先级最低
        if (_currentType == SYSTEM_STATE) {
            if (_currentState.mode != mode || 
                _currentState.color != color) {
                
                _currentState = {mode, color, brightness};
                updateLED();
            }
        }
    }

private:
    LEDManager() : _currentType(SYSTEM_STATE) {
        _currentState = {PWMLED::OFF, PWMLED::NONE, 0};
    }

    void updateLED() {
#ifdef PWM_LED_PIN
        pwmLed.setMode(_currentState.mode);
        pwmLed.setColor(_currentState.color);
        pwmLed.setBrightness(_currentState.brightness);
#endif
    }

    void setLEDState(PWMLED::Mode mode, PWMLED::ModuleColor color) {
        static PWMLED::Mode lastMode = PWMLED::OFF;
        static PWMLED::ModuleColor lastColor = PWMLED::NONE;

        // 只有状态发生变化时才更新
        if (lastMode != mode || lastColor != color) {
            pwmLed.setMode(mode);
            pwmLed.setColor(color);
            lastMode = mode;
            lastColor = color;
            
            // 打印状态变化日志
            Serial.printf("[LED] 状态更新 - 模式: %s, 颜色: %s\n", 
                modeToString(mode), colorToString(color));
        }
    }

    // 辅助函数：将模式转换为字符串（用于调试）
    const char* modeToString(PWMLED::Mode mode) {
        switch (mode) {
            case PWMLED::OFF: return "关闭";
            case PWMLED::SOLID: return "常亮";
            case PWMLED::BLINK_SLOW: return "慢闪";
            case PWMLED::BLINK_FAST: return "快闪";
            case PWMLED::BREATH: return "呼吸";
            default: return "未知";
        }
    }

    // 辅助函数：将颜色转换为字符串（用于调试）
    const char* colorToString(PWMLED::ModuleColor color) {
        switch (color) {
            case PWMLED::WHITE: return "白色(GPS)";
            case PWMLED::BLUE: return "蓝色(WiFi)";
            case PWMLED::YELLOW: return "黄色(4G)";
            case PWMLED::PURPLE: return "紫色(IMU)";
            case PWMLED::GREEN: return "绿色(系统正常)";
            case PWMLED::RED: return "红色(系统错误)";
            default: return "无色";
        }
    }

    StateType _currentType;
    LEDState _currentState;
};

// 全局访问点
#define ledManager LEDManager::getInstance()

#endif
