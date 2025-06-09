/*
 * ML307 简易 AT 驱动（适用于 Arduino 框架）
 * 仅实现基础 AT 通信、网络注册、拨号
 * 需要根据实际串口引脚修改
 */

#pragma once
#include <Arduino.h>

#define DEFAULT_BAUD_RATE 115200


class GsmManager {
public:
    GsmManager(HardwareSerial& serial, uint32_t baudrate, int8_t rxPin, int8_t txPin)
        : _serial(serial), _baudrate(baudrate), _rxPin(rxPin), _txPin(txPin) {}

    void begin() {
        _serial.begin(_baudrate, SERIAL_8N1, _rxPin, _txPin);
        delay(300);
    }

    // 发送AT命令并等待响应
    bool sendAT(const String& cmd, const String& expect = "OK", uint32_t timeout = 2000) {
        flushInput();
        _serial.println(cmd);
        uint32_t start = millis();
        String resp;
        while (millis() - start < timeout) {
            while (_serial.available()) {
                char c = _serial.read();
                resp += c;
                if (resp.indexOf(expect) != -1) {
                    return true;
                }
            }
        }
        return false;
    }

    // 网络注册
    bool waitForNetwork(uint32_t timeout = 10000) {
        uint32_t start = millis();
        while (millis() - start < timeout) {
            sendAT("AT+CREG?");
            delay(500);
            // 你可以在这里解析返回值，判断是否注册成功
            // 简化处理：假设OK即为成功
            if (sendAT("AT+CREG?", "0,1")) return true; // 0,1=已注册
        }
        return false;
    }

    // 拨号
    bool connectGPRS(const String& apn) {
        if (!sendAT("AT+CGATT=1")) return false;
        if (!sendAT("AT+CGDCONT=1,\"IP\",\"" + apn + "\"")) return false;
        if (!sendAT("AT+CGACT=1,1")) return false;
        return true;
    }

    // 发送原始AT命令，返回响应
    String sendRaw(const String& cmd, uint32_t timeout = 2000) {
        flushInput();
        _serial.println(cmd);
        uint32_t start = millis();
        String resp;
        while (millis() - start < timeout) {
            while (_serial.available()) {
                char c = _serial.read();
                resp += c;
            }
        }
        return resp;
    }

private:
    void flushInput() {
        while (_serial.available()) _serial.read();
    }

    HardwareSerial& _serial;
    uint32_t _baudrate;
    int8_t _rxPin, _txPin;
};