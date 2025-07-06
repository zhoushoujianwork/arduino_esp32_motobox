/*
 * ml307 MQTT 客户端实现
 * 参考ESP32 MQTT客户端设计模式
 * 增加网络状态检查和错误处理
 */

#include "Ml307Mqtt.h"
#include "../config.h"

Ml307Mqtt ml307Mqtt(ml307_at);

Ml307Mqtt::Ml307Mqtt(Ml307AtModem &modem)
    : _modem(modem), _isConnected(false), _canPublish(false),
      _lastCanPublishCheckTime(0), _connectStartTime(0),
      _debug(false), _port(0)
{
}

/**
 * 连接MQTT服务器 - 参考ESP32 MQTT连接流程
 * 1. 检查网络状态
 * 2. 断开现有连接
 * 3. 配置连接参数
 * 4. 发起连接
 * 5. 等待连接完成
 */
bool Ml307Mqtt::connect(const char *broker, uint16_t port,
                        const char *clientId,
                        const char *username,
                        const char *password)
{
    debugPrint("MQTT连接开始");

    // 保存连接参数
    _broker = broker ? broker : "";
    _port = port;
    _clientId = clientId ? clientId : "";
    _username = username ? username : "";
    _password = password ? password : "";

    // 1. 检查网络状态
    if (!_modem.isNetworkReady())
    {
        debugPrint("网络未就绪，无法连接MQTT");
        handleEvent(MQTT_EVENT_ERROR, "网络未就绪");
        return false;
    }

    // 检查信号强度
    int csq = _modem.getCSQ();
    if (csq < 10)
    {
        debugPrint("信号太弱，CSQ: " + String(csq));
        handleEvent(MQTT_EVENT_ERROR, "信号太弱");
        return false;
    }

    // 2. 断开现有连接
    if (_isConnected)
    {
        disconnect();
        delay(2000); // 等待断开完成
    }

    // 3. 重置连接状态
    resetConnectionState();
    _connectStartTime = millis();

    // 4. 先断开MQTT连接，确保状态为未连接
    _modem.sendATThreadSafe("AT+MQTTDISC=0");
    delay(1000);

    // 5. 检查MQTT状态，确保为未连接状态
    for (int i = 0; i < 10; ++i)
    {
        String resp = _modem.sendATWithResponseThreadSafe("AT+MQTTSTATE=0");
        if (resp.indexOf("+MQTTSTATE: 3") >= 0)
        {
            debugPrint("MQTT已处于未连接状态");
            break;
        }
        delay(500);
    }

    // 6. 配置SSL（如果需要）
    if (port == 8883)
    {
        if (!_modem.sendATWithRetryThreadSafe("AT+MQTTCFG=\"ssl\",0,1"))
        {
            debugPrint("设置SSL失败");
            handleEvent(MQTT_EVENT_ERROR, "SSL配置失败");
            return false;
        }
    }

    // 7. 配置clean session - 增加重试和错误处理
    bool cleanSessionOk = false;
    for (int retry = 0; retry < 3; retry++)
    {
        if (_modem.sendATWithRetryThreadSafe("AT+MQTTCFG=\"clean\",0,1"))
        {
            cleanSessionOk = true;
            break;
        }
        debugPrint("设置clean session失败，重试 " + String(retry + 1));
        delay(1000);
    }

    if (!cleanSessionOk)
    {
        debugPrint("设置clean session最终失败");
        handleEvent(MQTT_EVENT_ERROR, "Clean session配置失败");
        return false;
    }

    // 8. 配置keep alive
    if (!_modem.sendATWithRetryThreadSafe("AT+MQTTCFG=\"pingreq\",0,60"))
    {
        debugPrint("设置keep alive失败");
        handleEvent(MQTT_EVENT_ERROR, "Keep alive配置失败");
        return false;
    }

    // 9. 配置HEX编码
    if (!_modem.sendATWithRetryThreadSafe("AT+MQTTCFG=\"encoding\",0,1,1"))
    {
        debugPrint("设置HEX编码失败");
        handleEvent(MQTT_EVENT_ERROR, "HEX编码配置失败");
        return false;
    }

    // 10. 创建MQTT连接
    String cmd = String("AT+MQTTCONN=0,\"") + _broker + "\"," + String(_port) +
                 ",\"" + _clientId + "\",\"" + _username + "\",\"" + _password + "\"";
    if (!_modem.sendATWithRetryThreadSafe(cmd, "OK", 5, 15000))
    {
        debugPrint("MQTT连接指令失败");
        handleEvent(MQTT_EVENT_ERROR, "连接指令失败");
        return false;
    }

    // 11. 等待连接完成
    if (!waitForConnection())
    {
        debugPrint("MQTT连接超时");
        handleEvent(MQTT_EVENT_ERROR, "连接超时");
        return false;
    }

    // 12. 连接成功
    _isConnected = true;
    _canPublish = true;
    debugPrint("MQTT连接成功");
    handleEvent(MQTT_EVENT_CONNECTED);

    return true;
}

/**
 * 等待连接完成 - 参考ESP32的事件等待机制
 * 增加更详细的状态检查
 */
bool Ml307Mqtt::waitForConnection()
{
    unsigned long startTime = millis();
    int lastState = -1;

    while (millis() - startTime < MQTT_CONNECT_TIMEOUT_MS)
    {
        int currentState = getConnectionState();

        if (currentState != lastState)
        {
            debugPrint("MQTT状态变化: " + String(lastState) + " -> " + String(currentState));
            lastState = currentState;
        }

        // 只有状态1表示已连接，状态2是用户主动断开
        if (currentState == 1)
        {
            return true;
        }

        // 状态3表示未连接，继续等待
        if (currentState == 3)
        {
            delay(MQTT_STATE_CHECK_INTERVAL);
            continue;
        }

        // 其他状态可能是错误，等待一段时间再检查
        delay(1000);
    }

    return false;
}

/**
 * 获取连接状态
 */
int Ml307Mqtt::getConnectionState()
{
    String resp = _modem.sendATWithResponseThreadSafe("AT+MQTTSTATE=0");
    int state = -1;
    int idx = resp.indexOf("+MQTTSTATE: ");
    if (idx >= 0)
    {
        String stateStr = resp.substring(idx + 12);
        for (int i = 0; i < stateStr.length(); i++)
        {
            if (stateStr[i] >= '0' && stateStr[i] <= '9')
            {
                state = stateStr[i] - '0';
                break;
            }
        }
    }
    return state;
}

/**
 * 检查连接状态
 */
bool Ml307Mqtt::checkConnectionState()
{
    int state = getConnectionState();
    // 只有状态1表示已连接
    return (state == 1);
}

/**
 * 断开MQTT连接
 */
void Ml307Mqtt::disconnect()
{
    debugPrint("MQTT断开连接");
    _modem.sendATThreadSafe("AT+MQTTDISC=0");
    disconnectAndWait();
    handleEvent(MQTT_EVENT_DISCONNECTED);
}

/**
 * 重置连接状态
 */
void Ml307Mqtt::resetConnectionState()
{
    _isConnected = false;
    _canPublish = false;
    _lastCanPublishCheckTime = 0;
}

/**
 * 检查MQTT连接状态
 */
bool Ml307Mqtt::canPublish()
{
    if (!_isConnected)
    {
        return false;
    }

    // 定期检查连接状态
    if (millis() - _lastCanPublishCheckTime > 5000)
    {
        _lastCanPublishCheckTime = millis();
        _canPublish = checkConnectionState();

        // 如果连接状态检查失败，重置连接状态
        if (!_canPublish)
        {
            debugPrint("连接状态检查失败，重置MQTT状态");
            _isConnected = false;
        }
        else
        {
            String response = _modem.sendATWithResponseThreadSafe("AT+MQTTREAD?");
            if (response.indexOf("+MQTTREAD:") >= 0)
            {
                parseMessageData(response);
                debugPrint("MQTT读取到消息 " + response);
                handleEvent(MQTT_EVENT_MESSAGE);
            }
        }
    }

    return _canPublish;
}

/**
 * 断开MQTT连接并等待状态变为未连接
 */
void Ml307Mqtt::disconnectAndWait()
{
    _modem.sendATThreadSafe("AT+MQTTDISC=0");
    // 最多等待5秒，每500ms检查一次
    for (int i = 0; i < 10; ++i)
    {
        delay(500);
        String resp = _modem.sendATWithResponseThreadSafe("AT+MQTTSTATE=0");
        if (resp.indexOf("+MQTTSTATE: 3") >= 0)
        {
            debugPrint("MQTT已断开");
            break;
        }
    }
    resetConnectionState();
}

/**
 * 发布消息，带mqtt_id，HEX编码
 */
bool Ml307Mqtt::publish(const char *topic, const char *payload, bool retain)
{
    if (!canPublish())
    {
        debugPrint("MQTT未连接，无法发布消息");
        return false;
    }

    // 验证主题名称长度
    if (strlen(topic) > 64)
    {
        debugPrint("主题名称过长，最大64字符: " + String(topic));
        return false;
    }

    int qos = 0; // 固定使用QoS=0，避免CME错误
    int dup = 0;
    int messageId = 0;
    String hexPayload = encodeHex(payload);
    String cmd = "AT+MQTTPUB=0,\"" + String(topic) + "\"," + String(qos) + "," + String(retain ? 1 : 0) + "," + String(dup) + "," + String(strlen(payload)) + "," + hexPayload;

    bool result = _modem.sendATWithRetryThreadSafe(cmd, "OK", 3, 30000);
    if (!result)
    {
        debugPrint("MQTT发布消息失败");
        // 发布失败时检查连接状态
        if (!checkConnectionState())
        {
            debugPrint("发布失败，MQTT连接已断开");
            _isConnected = false;
            _canPublish = false;
        }
    }
    return result;
}

/**
 * 订阅主题，带mqtt_id
 */
bool Ml307Mqtt::subscribe(const char *topic, uint8_t qos)
{
    if (!canPublish())
    {
        debugPrint("MQTT未连接，无法订阅主题");
        return false;
    }

    // 验证主题名称长度
    if (strlen(topic) > 64)
    {
        debugPrint("主题名称过长，最大64字符: " + String(topic));
        return false;
    }

    // 限制QoS级别为0，避免CME错误
    uint8_t safeQos = (qos > 0) ? 0 : qos;

    String cmd = "AT+MQTTSUB=0,\"" + String(topic) + "\"," + String(safeQos);
    bool result = _modem.sendATWithRetryThreadSafe(cmd, "OK", 3, 10000);
    if (result)
    {
        debugPrint("MQTT订阅主题成功: " + String(topic));
    }
    else
    {
        debugPrint("MQTT订阅主题失败: " + String(topic));
        // 订阅失败时检查连接状态
        if (!checkConnectionState())
        {
            debugPrint("订阅失败，MQTT连接已断开");
            _isConnected = false;
            _canPublish = false;
        }
    }
    return result;
}

/**
 * 取消订阅，带mqtt_id
 */
bool Ml307Mqtt::unsubscribe(const char *topic)
{
    if (!canPublish())
    {
        debugPrint("MQTT未连接，无法取消订阅");
        return false;
    }

    String cmd = "AT+MQTTUNSUB=0,\"" + String(topic) + "\"";
    bool result = _modem.sendATThreadSafe(cmd);
    if (result)
    {
        debugPrint("MQTT取消订阅成功: " + String(topic));
    }
    else
    {
        debugPrint("MQTT取消订阅失败: " + String(topic));
    }
    return result;
}

/**
 * HEX编码工具
 */
String Ml307Mqtt::encodeHex(const char *payload)
{
    String hex;
    for (size_t i = 0; i < strlen(payload); ++i)
    {
        char buf[3];
        sprintf(buf, "%02X", (unsigned char)payload[i]);
        hex += buf;
    }
    return hex;
}

/**
 * 设置事件回调
 */
void Ml307Mqtt::onEvent(MqttEventCallback callback)
{
    _eventCallback = callback;
}

void Ml307Mqtt::onMessage(MqttMessageCallback callback)
{
    _messageCallback = callback;
}

bool Ml307Mqtt::connected()
{
    return _isConnected && _canPublish;
}

/**
 * 处理MQTT事件
 */
void Ml307Mqtt::handleEvent(MqttEventType event, const String &data)
{
    if (_eventCallback)
    {
        _eventCallback(event, data);
    }

    switch (event)
    {
    case MQTT_EVENT_CONNECTED:
        debugPrint("MQTT事件: 连接成功");
        break;
    case MQTT_EVENT_DISCONNECTED:
        debugPrint("MQTT事件: 连接断开");
        break;
    case MQTT_EVENT_ERROR:
        debugPrint("MQTT事件: 错误 - " + data);
        break;
    case MQTT_EVENT_MESSAGE:
        debugPrint("MQTT事件: 收到消息");
        break;
    }
}

// 4G 网络连接状态检查
void Ml307Mqtt::loop()
{
    if (!_isConnected)
    {
        return;
    }

    // 如果在获取lbs数据，则不进行mqtt的读取
    if (ml307_at.isLBSLoading())
    {
        debugPrint("LBS定位中,不进行MQTT读取");
        return;
    }

    String response = _modem.sendATWithResponseThreadSafe("AT+MQTTREAD?");
    if (response.indexOf("+MQTTREAD:") >= 0)
    {
        parseMessageData(response);
        debugPrint("MQTT读取到消息 " + response);
        handleEvent(MQTT_EVENT_MESSAGE);
    }
}
bool Ml307Mqtt::configureConnection()
{
    // 配置客户端ID
    String cmd = "AT+MQTTCFG=\"";
    cmd += _clientId;
    cmd += "\"";

    return _modem.sendATThreadSafe(cmd);
}

bool Ml307Mqtt::parseMessageData(const String &data)
{
    // 解析 +MQTTREAD: "topic","payload" 格式的数据
    int topicStart = data.indexOf("+MQTTREAD: \"");
    if (topicStart < 0)
        return false;

    topicStart += 11;
    int topicEnd = data.indexOf("\",\"", topicStart);
    if (topicEnd < 0)
        return false;

    String topic = data.substring(topicStart, topicEnd);

    int payloadStart = topicEnd + 3;
    int payloadEnd = data.indexOf("\"", payloadStart);
    if (payloadEnd < 0)
        return false;

    String payload = data.substring(payloadStart, payloadEnd);

    if (_messageCallback)
    {
        _messageCallback(topic, payload);
    }

    return true;
}

void Ml307Mqtt::handleMessage(const String &topic, const String &payload)
{
    if (_messageCallback)
    {
        _messageCallback(topic, payload);
    }
}

void Ml307Mqtt::debugPrint(const String &msg)
{
    if (_debug)
    {
        Serial.println("[ml307mqtt] [debug] " + msg);
    }
}

void Ml307Mqtt::setDebug(bool debug)
{
    _debug = debug;
}