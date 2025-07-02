#include "ExternalPower.h"
#include "../device.h"

#ifdef RTC_INT_PIN
ExternalPower externalPower(RTC_INT_PIN);
#endif

ExternalPower::ExternalPower(int pin)
    : _pin(pin),
      _is_connected(false),
      _last_state(false),
      _last_check_time(0),
      _last_change_time(0),
      _raw_state(false),
      _debug(false)
{
}

void ExternalPower::begin()
{
    pinMode(_pin, INPUT_PULLUP); // 初始化外部电源检测引脚
    
    // 初始读取状态
    _raw_state = readRawState();
    _is_connected = _raw_state;
    _last_state = _is_connected;
    _last_change_time = millis();
    
    // 更新设备状态
    device_state.external_power = _is_connected;
    
    debugPrint(String("外部电源检测初始化完成，引脚: ") + String(_pin) + 
               ", 初始状态: " + (_is_connected ? "已连接" : "未连接"));
}

void ExternalPower::loop()
{
    unsigned long current_time = millis();
    
    // 检查是否到了检查时间
    if (current_time - _last_check_time >= CHECK_INTERVAL)
    {
        _last_check_time = current_time;
        updateState();
    }
}

bool ExternalPower::readRawState()
{
    // RTC_INT_PIN: 输入 5V 拉低，无输入时候高电平
    // 所以 LOW 表示外部电源接入，HIGH 表示外部电源断开
    return (digitalRead(_pin) == LOW);
}

void ExternalPower::updateState()
{
    bool current_raw_state = readRawState();
    unsigned long current_time = millis();
    
    // 检测状态变化
    if (current_raw_state != _raw_state)
    {
        _raw_state = current_raw_state;
        _last_change_time = current_time;
        debugPrint(String("外部电源原始状态变化: ") + (current_raw_state ? "已连接" : "未连接"));
    }
    
    // 防抖处理：状态稳定一段时间后才更新
    if (current_time - _last_change_time >= DEBOUNCE_DELAY)
    {
        if (_raw_state != _is_connected)
        {
            _is_connected = _raw_state;
            
            // 更新设备状态
            device_state.external_power = _is_connected;
            
            debugPrint(String("外部电源状态确认变化: ") + (_is_connected ? "已连接" : "未连接"));
            
            // 可以在这里添加状态变化的回调处理
            if (_is_connected)
            {
                Serial.println("[外部电源] 车辆电门接入");
            }
            else
            {
                Serial.println("[外部电源] 车辆电门断开");
            }
        }
    }
}

bool ExternalPower::isConnected()
{
    return _is_connected;
}

void ExternalPower::debugPrint(const String& message)
{
    if (_debug)
    {
        Serial.println("[外部电源] " + message);
    }
}
