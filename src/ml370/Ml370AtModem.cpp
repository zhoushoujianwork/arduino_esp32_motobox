/*
 * ML370 AT Modem 驱动实现
 */

#include "Ml370AtModem.h"

Ml370AtModem ml370;

Ml370AtModem::Ml370AtModem(HardwareSerial& serial)
    : _serial(serial)
    , _debug(false) {
}

bool Ml370AtModem::begin(uint32_t baudrate) {
    _serial.begin(baudrate);
    delay(100);
    
    // 关闭回显
    sendAT("ATE0");
    
    // 检查模块是否响应
    if (!isReady()) {
        debugPrint("Module not responding");
        return false;
    }
    
    // 设置短信文本模式
    sendAT("AT+CMGF=1");
    
    return true;
}

void Ml370AtModem::setDebug(bool debug) {
    _debug = debug;
}

bool Ml370AtModem::waitForNetwork(uint32_t timeout) {
    uint32_t start = millis();
    
    while (millis() - start < timeout) {
        // 检查网络注册状态
        String response = sendATWithResponse("AT+CREG?");
        if (response.indexOf("+CREG: 0,1") >= 0 || response.indexOf("+CREG: 0,5") >= 0) {
            debugPrint("Network registered");
            return true;
        }
        delay(1000);
    }
    
    debugPrint("Network registration timeout");
    return false;
}

bool Ml370AtModem::reset() {
    return sendAT("AT+CFUN=1,1", "OK", 10000);
}

String Ml370AtModem::getLocalIP() {
    String response = sendATWithResponse("AT+CGPADDR=1");
    int start = response.indexOf("+CGPADDR: 1,\"");
    if (start >= 0) {
        start += 12;
        int end = response.indexOf("\"", start);
        if (end >= 0) {
            return response.substring(start, end);
        }
    }
    return "";
}

int Ml370AtModem::getCSQ() {
    String response = sendATWithResponse("AT+CSQ");
    int start = response.indexOf("+CSQ: ");
    if (start >= 0) {
        start += 6;
        return response.substring(start).toInt();
    }
    return -1;
}

String Ml370AtModem::getIMEI() {
    return sendATWithResponse("AT+CGSN");
}

String Ml370AtModem::getICCID() {
    String response = sendATWithResponse("AT+CCID");
    int start = response.indexOf("+CCID: ");
    if (start >= 0) {
        start += 7;
        return response.substring(start);
    }
    return "";
}

String Ml370AtModem::getModuleName() {
    return sendATWithResponse("AT+CGMM");
}

String Ml370AtModem::getCarrierName() {
    String response = sendATWithResponse("AT+COPS?");
    int start = response.indexOf("+COPS: 0,0,\"");
    if (start >= 0) {
        start += 11;
        int end = response.indexOf("\"", start);
        if (end >= 0) {
            return response.substring(start, end);
        }
    }
    return "";
}

bool Ml370AtModem::sendAT(const String& cmd, const String& expected, uint32_t timeout) {
    flushInput();
    
    debugPrint("> " + cmd);
    _serial.println(cmd);
    
    return expectResponse(expected, timeout);
}

String Ml370AtModem::sendATWithResponse(const String& cmd, uint32_t timeout) {
    flushInput();
    
    debugPrint("> " + cmd);
    _serial.println(cmd);
    
    return waitResponse(timeout);
}

bool Ml370AtModem::isReady() {
    return sendAT("AT");
}

void Ml370AtModem::flushInput() {
    while (_serial.available()) {
        _serial.read();
    }
}

String Ml370AtModem::waitResponse(uint32_t timeout) {
    String response;
    uint32_t start = millis();
    
    while (millis() - start < timeout) {
        if (_serial.available()) {
            char c = _serial.read();
            response += c;
            
            // 检查是否收到完整响应
            if (response.endsWith("\r\nOK\r\n") || 
                response.endsWith("\r\nERROR\r\n") ||
                response.endsWith("\r\n> ")) {
                break;
            }
        }
    }
    
    if (_debug) {
        debugPrint("< " + response);
    }
    
    return response;
}

bool Ml370AtModem::expectResponse(const String& expected, uint32_t timeout) {
    String response = waitResponse(timeout);
    return response.indexOf(expected) >= 0;
}

void Ml370AtModem::debugPrint(const String& msg) {
    if (_debug) {
        Serial.println(msg);
    }
} 