#include "Ml307AtModem.h"

#ifdef GSM_ENABLED
// 构造全局对象 ml370，默认使用 Serial2，波特率 115200，RX=17，TX=18
Ml307AtModem ml370(Serial2, 115200, GSM_RX_PIN, GSM_TX_PIN);

// 修改构造函数实现，添加resetPin参数
Ml307AtModem::Ml307AtModem(HardwareSerial& serial, uint32_t baudrate, int8_t rxPin, int8_t txPin, int8_t resetPin)
    : _serial(serial), _baudrate(baudrate), _rxPin(rxPin), _txPin(txPin), _resetPin(resetPin) {}

// 初始化串口并等待网络注册
void Ml307AtModem::begin() {
    Serial.println("[GSM] 开始初始化...");
    
    // 先尝试自动检测波特率
    if (!autoBaudrate()) {
        Serial.println("[GSM] 波特率检测失败，初始化终止");
        return;
    }
    
    // 确认AT可用
    Serial.println("[GSM] 测试AT响应...");
    bool atOk = false;
    uint32_t startTime = millis();
    
    while (!atOk && (millis() - startTime < 30000)) {
        if(sendAT("AT", "OK", 1000)) {
            atOk = true;
            Serial.println("[GSM] AT响应正常");
            break;
        }
        Serial.println("[GSM] AT测试失败，重试...");
        delay(1000);
    }
    
    if (!atOk) {
        Serial.println("[GSM] AT响应始终失败，初始化终止");
        return;
    }
    
    // 3. 切换到高波特率 921600
    Serial.println("[GSM] 切换到高波特率...");
    if(sendAT("AT+IPR=921600", "OK", 2000)) {
        _serial.flush();
        delay(100);
        _serial.begin(921600, SERIAL_8N1, _rxPin, _txPin);
        delay(1000);
        
        if(!sendAT("AT", "OK", 1000)) {
            Serial.println("[GSM] 高波特率切换失败，回退到115200");
            _serial.begin(115200, SERIAL_8N1, _rxPin, _txPin);
            delay(1000);
        } else {
            Serial.println("[GSM] 切换到921600波特率成功");
        }
    }

    // 4. 基础配置
    sendAT("ATE0", "OK", 1000);  // 关闭回显
    
    // 5. 检查SIM卡
    Serial.println("[GSM] 检查SIM卡...");
    if(!sendAT("AT+CPIN?", "+CPIN: READY", 5000)) {
        Serial.println("[GSM] SIM卡异常");
        return;
    }
    Serial.println("[GSM] SIM卡正常");

    // 6. 等待网络就绪
    Serial.println("[GSM] 等待网络就绪...");
    waitForNetwork();
}

// 等待网络注册
bool Ml307AtModem::waitForNetwork(uint32_t timeout) {
    // 设置网络模式，改为不重启
    if(!sendAT("AT+CFUN=1", "OK", 20000)) {  // 全功能模式，不重启
        Serial.println("[GSM] 设置全功能模式失败");
        return false;
    }
    delay(5000);  // 增加等待时间到5秒
    
    // 先设置CREG的主动上报模式
    if(!sendAT("AT+CREG=1", "OK", 2000)) {
        Serial.println("[GSM] 设置CREG模式失败");
    }
    
    // 增加网络注册等待时间
    uint32_t start = millis();
    while (millis() - start < 300000) { // 5 分钟超时
        // 检查网络注册状态
        String resp = sendRaw("AT+CREG?", 2000);
        
        // 先检查命令是否执行成功
        if(resp.indexOf("ERROR") >= 0) {
            Serial.println("[GSM] CREG查询失败，等待2秒后重试...");
            delay(2000);
            continue;
        }
        
        // 打印实际的resp结果
        Serial.println("[GSM] 网络注册状态: " + resp);
        
        // 检查注册状态
        if(resp.indexOf("+CREG: 1,1") >= 0 || resp.indexOf("+CREG: 1,5") >= 0) {
            Serial.println("[GSM] 网络注册成功");
            
            // 检查GPRS附着
            if(sendAT("AT+CGATT=1", "OK", 10000)) {
                Serial.println("[GSM] GPRS附着成功");
                return true;
            }
        } else if(resp.indexOf("+CREG: 1,2") >= 0) {
            Serial.println("[GSM] 正在搜索网络...");
        } else if(resp.indexOf("+CREG: 1,3") >= 0) {
            Serial.println("[GSM] 网络注册被拒绝");
        } else if(resp.indexOf("+CREG: 1,0") >= 0) {
            Serial.println("[GSM] 还未开始搜索网络");
        }
        
        delay(2000);  // 2秒检查间隔
    }
    
    Serial.println("[GSM] 网络注册超时");
    return false;
}

// 拨号，APN由运营商提供
bool Ml307AtModem::connectGPRS(const String& apn) {
    Serial.println("[GSM] 开始GPRS连接...");
    
    // 确保关闭之前的连接
    sendAT("AT+CIPSHUT", "SHUT OK", 5000);
    delay(1000);
    
    // 检查GPRS状态
    String gprsStatus = sendRaw("AT+CGATT?", 2000);
    Serial.println("[GSM] GPRS状态: " + gprsStatus);
    
    if (gprsStatus.indexOf("+CGATT: 1") == -1) {
        Serial.println("[GSM] 尝试GPRS附着...");
        if (!sendAT("AT+CGATT=1", "OK", 10000)) {
            Serial.println("[GSM] GPRS附着失败");
            return false;
        }
        delay(2000);
    }
    
    // 设置APN
    Serial.println("[GSM] 设置APN: " + apn);
    if (!sendAT("AT+CSTT=\"" + apn + "\"", "OK", 10000)) {
        Serial.println("[GSM] APN设置失败");
        return false;
    }
    delay(1000);
    
    // 激活无线链路
    Serial.println("[GSM] 激活无线链路...");
    if (!sendAT("AT+CIICR", "OK", 10000)) {
        Serial.println("[GSM] 无线链路激活失败");
        return false;
    }
    delay(1000);
    
    // 获取IP地址
    String ip = getLocalIP();
    if (ip.length() > 0) {
        Serial.println("[GSM] GPRS连接成功，IP: " + ip);
        return true;
    }
    
    Serial.println("[GSM] 获取IP地址失败");
    return false;
}

// 修改sendAT函数，减少日志输出
bool Ml307AtModem::sendAT(const String& cmd, const String& expect, uint32_t timeout) {
    String resp = sendRaw(cmd, timeout);
    
    if (cmd != "AT") {
        // 打印调试信息
    if (cmd.length() > 0) {
        Serial.print("[GSM] >>> ");
        Serial.println(cmd);
    }
    if (resp.length() > 0) {
        Serial.print("[GSM] <<< ");
        Serial.println(resp);
    }
    }
    
    // 检查响应
    if (resp.length() == 0) {
        Serial.print("[GSM] 命令超时: ");
        Serial.println(cmd);
        return false;
    }
    
    // 如果没有期望值，只要有响应就算成功
    if (expect.length() == 0) {
        return true;
    }
    
    // 检查是否包含期望的响应
    return resp.indexOf(expect) >= 0;
}

// 修改sendRaw函数，减少日志输出
String Ml307AtModem::sendRaw(const String& cmd, uint32_t timeout) {
    _responseBuffer = "";  // 清空响应缓存
    flushInput();
    
    // 发送命令
    _serial.println(cmd);
    
    // 等待响应
    uint32_t start = millis();
    while (millis() - start < timeout) {
        while (_serial.available()) {
            char c = _serial.read();
            if (_rxBufferLength < RX_BUFFER_SIZE) {
                _rxBuffer[_rxBufferLength++] = c;
            }
            _responseBuffer += c;  // 同时存入响应缓存
        }
        
        // 检查是否有完整响应
        if (_responseBuffer.endsWith("\r\n") || 
            _responseBuffer.endsWith("OK\r\n") || 
            _responseBuffer.endsWith("ERROR\r\n")) {
            break;
        }
    }
    
    return _responseBuffer;
}

// 获取本地IP地址
String Ml307AtModem::getLocalIP() {
    String resp = sendRaw("AT+CGPADDR=1", 2000);
    int idx = resp.indexOf(":");
    if (idx != -1) {
        int start = resp.indexOf(",", idx);
        if (start != -1) {
            int end = resp.indexOf("\r", start);
            if (end != -1) {
                return resp.substring(start + 1, end);
            }
        }
    }
    return "";
}

// 获取信号强度
int Ml307AtModem::getCSQ() {
    String resp = sendRaw("AT+CSQ", 2000);
    int idx = resp.indexOf(":");
    if (idx != -1) {
        int comma = resp.indexOf(",", idx);
        if (comma != -1) {
            String val = resp.substring(idx + 1, comma);
            return val.toInt();
        }
    }
    return -1;
}

// 获取IMEI
String Ml307AtModem::getIMEI() {
    String resp = sendRaw("AT+GSN", 2000);
    int idx = resp.indexOf("\r\n");
    if (idx != -1) {
        int end = resp.indexOf("\r\n", idx + 2);
        if (end != -1) {
            return resp.substring(idx + 2, end);
        }
    }
    return "";
}

// 获取ICCID
String Ml307AtModem::getICCID() {
    String resp = sendRaw("AT+ICCID", 2000);
    int idx = resp.indexOf(":");
    if (idx != -1) {
        int end = resp.indexOf("\r", idx);
        if (end != -1) {
            return resp.substring(idx + 1, end);
        }
    }
    return "";
}

// 重置模块
bool Ml307AtModem::resetModem() {
    Serial.println("[GSM] 正在重置模块...");
    
    // 先测试AT响应
    Serial.println("[GSM] 测试AT响应...");
    for(int i=0; i<3; i++) {
        if (sendAT("AT", "OK", 1000)) {
            Serial.println("[GSM] AT响应正常");
            break;
        }
        Serial.println("[GSM] AT响应失败，重试...");
        delay(1000);
    }
    
    // 获取当前模块状态
    String resp = sendRaw("AT+CFUN?", 2000);
    Serial.println("[GSM] 当前模块状态: " + resp);
    
    // 退出可能的数据模式
    Serial.println("[GSM] 尝试退出数据模式...");
    delay(1000);
    _serial.write("+++");
    delay(1000);
    
    // 重置前先关闭所有连接
    Serial.println("[GSM] 关闭所有连接...");
    sendAT("AT+CIPSHUT", "OK", 5000);
    
    // 重置模块
    Serial.println("[GSM] 发送重置命令...");
    if (!sendAT("AT+CFUN=1,1", "OK", 2000)) {
        Serial.println("[GSM] 重置命令失败");
        return false;
    }
    
    // 增加重置后等待时间
    Serial.println("[GSM] 等待模块重启...");
    delay(15000);  // 增加到15秒
    
    // 检查模块是否就绪
    Serial.println("[GSM] 检查模块状态...");
    bool ready = false;
    for(int i=0; i<5; i++) {
        if (sendAT("AT", "OK", 1000)) {
            Serial.println("[GSM] 模块重置成功");
            ready = true;
            break;
        }
        Serial.println("[GSM] 等待模块就绪...");
        delay(1000);
    }
    
    if (!ready) {
        Serial.println("[GSM] 模块重置失败");
        return false;
    }
    
    // 重新配置一些基本设置
    sendAT("AT+CMEE=2", "OK", 1000);  // 启用详细错误信息
    sendAT("AT+CIPMODE=0", "OK", 1000);  // 设置非透传模式
    
    return true;
}

// 检查网络状态
bool Ml307AtModem::checkNetworkStatus() {
    Serial.println("[GSM] 检查网络状态...");
    
    // 检查SIM卡
    if (!sendAT("AT+CPIN?", "READY", 1000)) {
        Serial.println("[GSM] SIM卡异常");
        return false;
    }
    
    // 检查信号强度
    if (!sendAT("AT+CSQ", "OK", 1000)) {
        Serial.println("[GSM] 无信号");
        return false;
    }
    
    // 检查网络注册
    if (!sendAT("AT+CGATT?", "+CGATT: 1", 2000)) {
        Serial.println("[GSM] 网络未注册");
        return false;
    }
    
    Serial.println("[GSM] 网络状态正常");
    return true;
}

// 修改connect函数，添加MQTT数据发送日志
size_t Ml307AtModem::write(const uint8_t *buf, size_t size) {
    if (!_connected) {
        Serial.println("[MQTT] 网络未连接，发送失败");
        return 0;
    }

    // 输出MQTT数据内容（十六进制格式）
    Serial.print("[MQTT] 发送数据(HEX): ");
    for(size_t i = 0; i < min(size, (size_t)32); i++) {  // 只显示前32字节
        if(buf[i] < 16) Serial.print("0");
        Serial.print(buf[i], HEX);
        Serial.print(" ");
    }
    if(size > 32) Serial.print("...");
    Serial.println();

    const size_t MAX_CHUNK_SIZE = 512;
    size_t sent = 0;

    while (sent < size) {
        size_t chunk_size = min(size - sent, MAX_CHUNK_SIZE);
        String cmd = "AT+CIPSEND=0," + String(chunk_size);
        
        if (!sendAT(cmd.c_str(), ">", 5000)) {
            Serial.println("[MQTT] 数据发送准备失败");
            return sent;
        }

        _serial.write(buf + sent, chunk_size);
        
        String resp;
        uint32_t start = millis();
        bool sendOk = false;
        while (millis() - start < 10000) {
            if (_serial.available()) {
                char c = _serial.read();
                resp += c;
                if (resp.indexOf("SEND OK") != -1) {
                    sendOk = true;
                    break;
                }
            }
            delay(10);
        }

        if (!sendOk) {
            Serial.println("[MQTT] 数据块发送超时");
            return sent;
        }

        sent += chunk_size;
        delay(100);
    }

    Serial.println("[MQTT] 数据发送成功，总字节数: " + String(sent));
    return sent;
}

int Ml307AtModem::read() {
    if (_rxBufferIndex >= _rxBufferLength) {
        _rxBufferIndex = 0;
        _rxBufferLength = 0;
        processRxData();  // 尝试读取新数据
        if (_rxBufferLength == 0) return -1;
    }
    return _rxBuffer[_rxBufferIndex++];
}

int Ml307AtModem::read(uint8_t *buf, size_t size) {
    size_t count = 0;
    while (count < size) {
        int c = read();
        if (c == -1) break;
        buf[count++] = (uint8_t)c;
    }
    return count;
}

// 实现 flushInput
void Ml307AtModem::flushInput() {
    while (_serial.available()) {
        _serial.read();
    }
    _rxBufferIndex = 0;
    _rxBufferLength = 0;
    _responseBuffer = "";
}

// 实现 stop
void Ml307AtModem::stop() {
    if (_connected) {
        sendAT("AT+CIPCLOSE=0", "OK", 2000);
        _connected = false;
    }
}

// 添加 IPAddress 版本的 connect 实现
int Ml307AtModem::connect(IPAddress ip, uint16_t port) {
    char ipStr[16];
    sprintf(ipStr, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
    return connect(ipStr, port);
}

// 添加单字节版本的 write 实现
size_t Ml307AtModem::write(uint8_t b) {
    return _serial.write(b);
}

// 添加 available 实现
int Ml307AtModem::available() {
    // 先处理缓存中的数据
    if (_rxBufferIndex < _rxBufferLength) {
        return _rxBufferLength - _rxBufferIndex;
    }
    
    // 再检查串口
    return _serial.available();
}

// 添加 peek 实现
int Ml307AtModem::peek() {
    if (_rxBufferIndex >= _rxBufferLength) {
        _rxBufferIndex = 0;
        _rxBufferLength = 0;
        processRxData();  // 尝试读取新数据
        if (_rxBufferLength == 0) return -1;
    }
    return _rxBuffer[_rxBufferIndex];
}

// 添加 flush 实现
void Ml307AtModem::flush() {
    _rxBufferIndex = 0;
    _rxBufferLength = 0;
    while (sendRaw("AT+CIPRXGET=2,0,1024", 1000).indexOf("+CIPRXGET: 2,") != -1) {
        // 清空所有未读数据
        delay(100);
    }
}

// 添加 connected 实现
uint8_t Ml307AtModem::connected() {
    return _connected ? 1 : 0;
}

// 添加 bool 操作符实现
Ml307AtModem::operator bool() {
    return connected();
}

// 修改processRxData函数，添加MQTT接收数据日志
void Ml307AtModem::processRxData() {
    if (!_connected) return;

    String resp = sendRaw("AT+CIPRXGET=4", 1000);
    if (resp.indexOf("+CIPRXGET: 4,0,") == -1) return;

    resp = sendRaw("AT+CIPRXGET=2,0,1024", 2000);
    if (resp.indexOf("+CIPRXGET: 2,") != -1) {
        int start = resp.indexOf(",") + 1;
        int end = resp.indexOf(",", start);
        int len = resp.substring(start, end).toInt();
        
        if (_rxBufferLength + len <= RX_BUFFER_SIZE) {
            // 输出MQTT接收数据日志
            Serial.print("[MQTT] 接收数据(HEX): ");
            const uint8_t* data = (const uint8_t*)(resp.c_str() + resp.lastIndexOf("\n") + 1);
            for(int i = 0; i < min(len, 32); i++) {  // 只显示前32字节
                if(data[i] < 16) Serial.print("0");
                Serial.print(data[i], HEX);
                Serial.print(" ");
            }
            if(len > 32) Serial.print("...");
            Serial.println();
            
            memcpy(_rxBuffer + _rxBufferLength, data, len);
            _rxBufferLength += len;
            Serial.println("[MQTT] 数据接收成功，总字节数: " + String(len));
        } else {
            Serial.println("[MQTT] 接收缓冲区溢出，数据丢失");
        }
    }
}

// isReady
bool Ml307AtModem::isReady() {
    return _connected;
}

// 修改connect函数，添加MQTT数据发送日志
int Ml307AtModem::connect(const char *host, uint16_t port) {
    // 确保关闭之前的连接
    stop();
    delay(1000);
    
    // 先进行DNS解析
    Serial.println("[GSM] 解析域名: " + String(host));
    String cmd = "AT+CDNSGIP=\"" + String(host) + "\"";
    String resp = sendRaw(cmd.c_str(), 20000);
    
    // 如果是IP地址，直接使用
    IPAddress ip;
    String target = ip.fromString(host) ? host : extractIP(resp);
    
    if (target.length() == 0) {
        Serial.println("[GSM] 域名解析失败");
        return 0;
    }
    
    // 打印解析结果
    Serial.println("[GSM] 域名解析结果 IP: " + target);
    
    // 建立连接
    cmd = "AT+CIPSTART=\"TCP\",\"" + target + "\"," + String(port);
    if (!sendAT(cmd.c_str(), "CONNECT OK", 20000)) {
        Serial.println("[GSM] TCP连接失败");
        return 0;
    }
    
    _connected = true;
    return 1;
}

// 实现extractIP函数
String Ml307AtModem::extractIP(const String& response) {
    int start = response.indexOf("+CDNSGIP: 1,\"");
    if (start != -1) {
        start += 12;
        int end = response.indexOf("\"", start);
        if (end != -1) {
            return response.substring(start, end);
        }
    }
    return "";
}

int Ml307AtModem::parseCSQ(const String& response) {
    int start = response.indexOf("+CSQ: ");
    if (start != -1) {
        start += 6;
        int end = response.indexOf(",", start);
        if (end != -1) {
            return response.substring(start, end).toInt();
        }
    }
    return -1;
}

bool Ml307AtModem::autoBaudrate() {
    // 常用波特率列表，按照最可能的顺序排列
    const uint32_t baudrates[] = {115200, 9600, 921600, 460800, 57600, 38400, 19200};
    
    Serial.println("[GSM] 开始自动检测波特率...");
    
    for(uint32_t baudrate : baudrates) {
        Serial.printf("[GSM] 尝试波特率: %d\n", baudrate);
        
        // 切换波特率
        _serial.begin(baudrate, SERIAL_8N1, _rxPin, _txPin);
        delay(100);
        
        // 清空缓冲区
        flushInput();
        
        // 多次尝试AT命令
        for(int i = 0; i < 3; i++) {
            _serial.println("AT");  // 直接发送，不通过sendAT
            delay(100);
            
            String response;
            uint32_t start = millis();
            
            // 等待响应
            while(millis() - start < 500) {  // 500ms超时
                while(_serial.available()) {
                    char c = _serial.read();
                    response += c;
                    if(response.indexOf("OK") >= 0) {
                        Serial.printf("[GSM] 找到正确波特率: %d\n", baudrate);
                        _baudrate = baudrate;
                        return true;
                    }
                }
                delay(10);
            }
        }
    }
    
    Serial.println("[GSM] 未能找到正确波特率");
    return false;
}
#endif
