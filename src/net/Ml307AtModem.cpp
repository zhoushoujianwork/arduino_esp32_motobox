/*
 * ml307 AT Modem 驱动实现
 */

#include "Ml307AtModem.h"

#ifdef ENABLE_GSM
Ml307AtModem ml307(Serial1, GSM_RX_PIN, GSM_TX_PIN);
#else
Ml307AtModem ml307(nullptr, 0, 0);
#endif

Ml307AtModem::Ml307AtModem(HardwareSerial& serial, int rxPin, int txPin)
    : _serial(serial)
    , _rxPin(rxPin)
    , _txPin(txPin)
    , _debug(false) {
}

bool Ml307AtModem::begin(uint32_t baudrate) {
    debugPrint("开始初始化ml307模块...");
    
    // 使用固定波特率初始化
    _serial.begin(baudrate, SERIAL_8N1, _rxPin, _txPin);
    delay(1000); // 增加延时，等待模块稳定
    
    // 清空缓冲区
    flushInput();
    
    while (!isReady()) {
        debugPrint("等待模块就绪...");
        delay(2000); // 增加等待时间
    }
    
    // 必须成功初始化
    while (!initModem()) {
        debugPrint("重试初始化模块...");
        delay(1000);
    }
    
    return true;
}

// 尝试特定波特率
bool Ml307AtModem::tryBaudrate(uint32_t baudrate) {
    _serial.begin(baudrate, SERIAL_8N1, _rxPin, _txPin);  // 指定引脚和串口配置
    delay(100);
    
    // 清空缓冲区
    while (_serial.available()) {
        _serial.read();
    }
    
    // 多次尝试AT测试
    for (int i = 0; i < 3; i++) {
        debugPrint("> AT");
        _serial.println("AT");
        
        // 等待响应
        String response = waitResponse(1000);
        debugPrint("< " + response);
        
        if (response.indexOf("OK") >= 0) {
            return true;
        }
        delay(100);
    }
    
    return false;
}

// 封装一个带重试的AT命令发送
bool Ml307AtModem::sendATWithRetry(const String& cmd, const String& expected, int maxRetry, uint32_t timeout) {
    for (int i = 0; i < maxRetry; ++i) {
        if (sendAT(cmd, expected, timeout)) {
            return true;
        }
        delay(500);
    }
    return false;
}

// 模块初始化配置
bool Ml307AtModem::initModem() {
    delay(2000);

    // 关闭回显
    if (!sendATWithRetry("ATE0")) {
        debugPrint("关闭回显失败");
        return false;
    }

    // 启用网络注册状态主动上报
    if (!sendATWithRetry("AT+CEREG=1")) {
        debugPrint("设置CEREG失败");
        return false;
    }
    

    // 检查 PIN 码状态
    int pinRetry = 0;
    const int maxPinRetry = 5;
    String pinStatus;
    do {
        pinStatus = sendATWithResponse("AT+CPIN?");
        if (pinStatus.indexOf("READY") >= 0) break;
        delay(500);
        pinRetry++;
    } while (pinRetry < maxPinRetry);
    if (pinStatus.indexOf("READY") < 0) {
        debugPrint("SIM卡 PIN 码未就绪");
        return false;
    }

    // 检查信号强度
    int csqRetry = 0;
    int csq = -1;
    do {
        csq = getCSQ();
        if (csq >= 5) break;
        delay(1000);
        csqRetry++;
    } while (csqRetry < 5);
    if (csq < 5) {
        debugPrint("信号太弱，无法注册网络");
        return false;
    }

    // 激活PDP上下文
    if (!sendATWithRetry("AT+MIPCALL?", "OK", 5, 5000)) {
        debugPrint("激活PDP上下文失败");
        return false;
    }

    // 等待网络注册
    while (!isNetworkReady()) {
        debugPrint("等待网络注册...");
        delay(2000);
    }

    return true;
}

void Ml307AtModem::setDebug(bool debug) {
    _debug = debug;
}

/*
1,0	未注册；ME 当前没有搜索要注册业务的新运营商
1,1	已注册，本地网
1,2	未注册，但 ME 正在搜索要注册业务的新运营商
1,3	注册被拒绝
1,4	未知(超出E-UTRAN网覆盖范围)
1,5	注册漫游网
*/
bool Ml307AtModem::isNetworkReady() {
    String pdpResponse = sendATWithResponse("AT+CEREG?");
    debugPrint("CEREG状态: " + pdpResponse);

    // 查找状态码
    int idx = pdpResponse.indexOf("+CEREG:");
    if (idx >= 0) {
        // 跳过 "+CEREG: " 找到状态值
        idx = pdpResponse.indexOf(',', idx);
        if (idx >= 0 && pdpResponse.length() > idx + 1) {
            char stat = pdpResponse.charAt(idx + 1);
            String statusMsg;
            switch (stat) {
                case '0': statusMsg = "未注册，未搜索新运营商"; break;
                case '1': statusMsg = "已注册，本地网"; break;
                case '2': statusMsg = "未注册，正在搜索新运营商"; break;
                case '3': statusMsg = "注册被拒绝"; break;
                case '4': statusMsg = "未知（超出覆盖范围）"; break;
                case '5': statusMsg = "已注册，漫游网"; break;
                default:  statusMsg = "未知状态"; break;
            }
            debugPrint("CEREG注册状态: " + String(stat) + " - " + statusMsg);
            return stat == '1' || stat == '5';
        }
    }
    
    debugPrint("CEREG响应格式异常");
    return false;
}

bool Ml307AtModem::reset() {
    return sendAT("AT+CFUN=1,1", "OK", 10000);
}

String Ml307AtModem::getLocalIP() {
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

int Ml307AtModem::getCSQ() {
    String response = sendATWithResponse("AT+CSQ");
    int start = response.indexOf("+CSQ: ");
    if (start >= 0) {
        start += 6;
        return response.substring(start).toInt();
    }
    return -1;
}

String Ml307AtModem::getIMEI() {
    return sendATWithResponse("AT+CGSN");
}

String Ml307AtModem::getICCID() {
    String response = sendATWithResponse("AT+CCID");
    int start = response.indexOf("+CCID: ");
    if (start >= 0) {
        start += 7;
        return response.substring(start);
    }
    return "";
}

String Ml307AtModem::getModuleName() {
    return sendATWithResponse("AT+CGMM");
}

String Ml307AtModem::getCarrierName() {
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

bool Ml307AtModem::sendAT(const String& cmd, const String& expected, uint32_t timeout) {
    flushInput();
    
    debugPrint("> " + cmd);
    _serial.println(cmd);
    
    return expectResponse(expected, timeout);
}

String Ml307AtModem::sendATWithResponse(const String& cmd, uint32_t timeout) {
    flushInput();
    
    debugPrint("> " + cmd);
    _serial.println(cmd);
    
    return waitResponse(timeout);
}

bool Ml307AtModem::isReady() {
    return sendAT("AT");
}

void Ml307AtModem::flushInput() {
    while (_serial.available()) {
        _serial.read();
    }
}

String Ml307AtModem::waitResponse(uint32_t timeout) {
    String response;
    uint32_t start = millis();
    
    while (millis() - start < timeout) {
        if (_serial.available()) {
            char c = _serial.read();
            response += c;
            
            // 检查是否收到完整响应，增加更多可能的结束标记
            if (response.endsWith("\r\nOK\r\n") || 
                response.endsWith("\r\nERROR\r\n") ||
                response.endsWith("\r\n> ") ||
                response.endsWith("+MATREADY\r\n") ||  // 处理特殊响应
                response.endsWith("\"CONNECT\"\r\n")) {
                break;
            }
        }
    }
    
    if (_debug) {
        if (response.length() == 0) {
            debugPrint("AT响应超时");
        } else {
            debugPrint("AT响应: " + response);
        }
    }
    
    return response;
}

bool Ml307AtModem::expectResponse(const String& expected, uint32_t timeout) {
    String response = waitResponse(timeout);
    return response.indexOf(expected) >= 0;
}

void Ml307AtModem::debugPrint(const String& msg) {
    if (_debug) {
        Serial.println("[ml307] [debug] " + msg);
    }
} 