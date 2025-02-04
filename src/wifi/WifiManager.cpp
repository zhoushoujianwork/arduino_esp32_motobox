#include "WifiManager.h"

WiFiConfigManager wifiManager;

void WiFiConfigManager::begin()
{
#if Enable_WIFI
    Serial.println("WiFiConfigManager::begin");
    delay(1000); // 添加启动延迟

    // 配置SSL客户端
    wifiClientSecure.setInsecure();  // 允许自签名证书
    wifiClientSecure.setTimeout(15); // 设置15秒超时

    if (!preferences.begin(PREF_NAMESPACE, false))
    {
        Serial.println("初始化配置存储失败");
        Serial.println("进入配置模式");
        enterConfigMode();
        return;
    }
    Serial.println("配置存储初始化成功");
#endif
}

void WiFiConfigManager::loop()
{
#if Enable_WIFI
    if (isConfigMode)
    {
        // 处理WiFi客户端
        handleClient();
    }
    else
    {
        if (WiFi.status() != WL_CONNECTED)
        {
            // 尝试连接
            if (tryConnectWithSavedCredentials())
            {
                Serial.println("\nWiFi连接成功!");
                device.set_wifi_connected(true);
            }
            else
            {
                Serial.println("\nWiFi连接失败,稍后重试");
                device.set_wifi_connected(false);
            }
        }
    }
#endif
}
bool WiFiConfigManager::tryConnectWithSavedCredentials()
{
    String ssid = preferences.getString("ssid", "");
    String password = preferences.getString("password", "");
    Serial.printf("尝试连接WiFi: %s\n", ssid.c_str());
    Serial.printf("尝试连接WiFi密码: %s\n", password.c_str());

    if (ssid.isEmpty())
    {
        Serial.println("未找到已保存的WiFi配置");
        Serial.println("进入配置模式");
        enterConfigMode();
        return false;
    }

    // 确保WiFi模式正确
    WiFi.mode(WIFI_STA);
    WiFi.disconnect(true);
    delay(100);

    // 设置WiFi事件处理
    WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info)
                 {
        switch(event) {
            case SYSTEM_EVENT_STA_DISCONNECTED:
                Serial.println("WiFi连接断开");
                WiFi.disconnect(true);
                device.get_device_state()->wifiConnected = false;
                break;
            case SYSTEM_EVENT_STA_GOT_IP:
                Serial.printf("获取到IP地址: %s\n", WiFi.localIP().toString().c_str());
                device.get_device_state()->wifiConnected = true;
                break;
            default:
                break;
        } });

    device.set_wifi_connected(false);
    // 开始连接
    WiFi.begin(ssid.c_str(), password.c_str());

    // 等待连接
    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED)
    {
        if (millis() - startTime > CONNECT_TIMEOUT_MS)
        {
            Serial.println("WiFi连接超时");
            WiFi.disconnect(true);
            return false;
        }
        delay(100); // 减少延时时间
        Serial.print(".");
    }

    // 验证互联网连接
    while (!checkInternetConnection())
    {
        Serial.println("无法访问互联网,5秒后重试...");
        delay(5000);
    }
    device.set_wifi_connected(true);

    return true;
}

void WiFiConfigManager::enterConfigMode()
{
    WiFi.disconnect(true); // 确保断开任何现有连接
    delay(100);

    isConfigMode = true;
    setupAP();
    setupDNS();
    setupWebServer();
}

void WiFiConfigManager::setupAP()
{
    WiFi.disconnect(true);
    delay(100);

    apSSID = String(AP_PREFIX) + device.get_device_id();

    WiFi.mode(WIFI_AP);
    delay(100);

    if (WiFi.softAP(apSSID.c_str(), AP_PASSWORD))
    {
        Serial.printf("启动配置热点成功: %s\n", apSSID.c_str());
        Serial.printf("AP IP地址: %s\n", WiFi.softAPIP().toString().c_str());
    }
    else
    {
        Serial.println("启动配置热点失败");
    }
}

void WiFiConfigManager::setupDNS()
{
    dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    if (dnsServer.start(53, "*", WiFi.softAPIP()))
    {
        Serial.println("DNS服务器启动成功");
    }
}

void WiFiConfigManager::setupWebServer()
{
    // 处理所有请求
    server.on("/", HTTP_GET, [this]()
              { server.send(200, "text/html", getConfigPage()); });

    server.on("/configure", HTTP_POST, [this]()
              { handleConfigSubmit(); });

    // 处理Captive Portal所需的特殊URL
    server.on("/generate_204", HTTP_GET, [this]()
              {
        server.sendHeader("Location", String("http://") + WiFi.softAPIP().toString(), true);
        server.send(302, "text/plain", ""); });

    server.on("/fwlink", HTTP_GET, [this]()
              {
        server.sendHeader("Location", String("http://") + WiFi.softAPIP().toString(), true);
        server.send(302, "text/plain", ""); });

    // 所有未处理的请求都重定向到配置页面
    server.onNotFound([this]()
                      {
        if (captivePortalRequest())
        { // 如果是Captive Portal请求
            server.sendHeader("Location", String("http://") + WiFi.softAPIP().toString(), true);
            server.send(302, "text/plain", "");
        }
        else
        {
            server.send(200, "text/html", getConfigPage());
        } });

    server.begin();
    Serial.println("配置服务器已启动");
}

void WiFiConfigManager::handleConfigSubmit()
{
    String newSSID = server.arg("ssid");
    String newPassword = server.arg("password");

    Serial.printf("接收到的SSID: %s\n", newSSID.c_str());
    Serial.printf("接收到的密码: %s\n", newPassword.c_str());

    if (newSSID.isEmpty())
    {
        server.send(400, "text/plain", "SSID不能为空");
        return;
    }

    saveWiFiCredentials(newSSID, newPassword);
    server.send(200, "text/html", "success! restarting...");
    delay(1000);
    ESP.restart();
}

void WiFiConfigManager::handleClient()
{
    if (isConfigMode)
    {
        dnsServer.processNextRequest();
        server.handleClient();
    }
}

String WiFiConfigManager::getConfigPage()
{
    String html = R"(
        <!DOCTYPE html>
        <html>
        <head>
            <meta charset='utf-8'>
            <meta name='viewport' content='width=device-width, initial-scale=1'>
            <title>MotoBox WiFi配置</title>
            <style>
                body { 
                    font-family: Arial, sans-serif;
                    padding: 20px;
                    background-color: #f5f5f5;
                    max-width: 600px;
                    margin: 0 auto;
                }
                .container {
                    background: white;
                    padding: 20px;
                    border-radius: 10px;
                    box-shadow: 0 2px 5px rgba(0,0,0,0.1);
                }
                .logo {
                    text-align: center;
                    margin-bottom: 20px;
                }
                .logo h1 {
                    color: #333;
                    font-size: 24px;
                }
                .logo span {
                    color: #666;
                    font-size: 14px;
                }
                input {
                    width: 100%;
                    padding: 10px;
                    margin: 10px 0;
                    border: 1px solid #ddd;
                    border-radius: 5px;
                    box-sizing: border-box;
                }
                button {
                    width: 100%;
                    padding: 12px;
                    background-color: #007bff;
                    color: white;
                    border: none;
                    border-radius: 5px;
                    cursor: pointer;
                    font-size: 16px;
                    margin-top: 15px;
                }
                button:hover {
                    background-color: #0056b3;
                }
                .footer {
                    text-align: center;
                    margin-top: 20px;
                    color: #666;
                    font-size: 12px;
                }
            </style>
        </head>
        <body>
            <div class="container">
                <div class="logo">
                    <h1>MotoBox</h1>
                    <span>智能摩托车数据盒子</span>
                </div>
                <form method='post' action='/configure'>
                    <input type='text' name='ssid' placeholder='WiFi名称' required>
                    <input type='password' name='password' placeholder='WiFi密码'>
                    <button type='submit'>连接网络</button>
                </form>
                <div class="footer">
                    <p>© 2024 MotoBox. 请连接到您的WiFi网络以继续使用。</p>
                </div>
            </div>
        </body>
        </html>
    )";
    return html;
}

void WiFiConfigManager::saveWiFiCredentials(const String &ssid, const String &password)
{
    if (preferences.clear())
    {
        preferences.putString("ssid", ssid);
        preferences.putString("password", password);
        Serial.printf("WiFi凭据保存成功: %s, %s\n", ssid.c_str(), password.c_str());
    }
    else
    {
        Serial.println("WiFi凭据保存失败");
    }
}

bool WiFiConfigManager::checkInternetConnection()
{
    // 使用IP地址替代域名来避免DNS解析问题
    IPAddress serverIP(220, 181, 38, 148); // baidu.com的一个IP地址
    const uint16_t port = 80;

    Serial.printf("正在验证互联网连接...\n");

    WiFiClient client;
    if (!client.connect(serverIP, port))
    {
        Serial.println("互联网连接失败！");
        return false;
    }

    // 发送 HTTP 请求
    client.print("GET / HTTP/1.1\r\n"
                 "Host: www.baidu.com\r\n"
                 "Connection: close\r\n\r\n");

    // 等待响应，设置更长的超时时间（10秒）
    unsigned long timeout = millis();
    while (client.available() == 0)
    {
        if (millis() - timeout > 10000)
        {
            Serial.println("请求超时！");
            client.stop();
            return false;
        }
        delay(100); // 添加短暂延时，减少CPU负载
    }

    // 如果有响应数据，则说明连接成功
    if (client.available())
    {
        Serial.println("互联网连接成功！");
        client.stop();
        return true;
    }

    client.stop();
    return false;
}

// 添加新的辅助方法来检测Captive Portal请求
bool WiFiConfigManager::captivePortalRequest()
{
    String host = server.hostHeader();
    if (host.length() > 0 && !isIp(host))
    {
        Serial.println("Captive Portal 请求: " + host);
        return true;
    }
    return false;
}

// 添加新的辅助方法来检查是否是IP地址
bool WiFiConfigManager::isIp(String str)
{
    for (size_t i = 0; i < str.length(); i++)
    {
        int c = str.charAt(i);
        if (c != '.' && (c < '0' || c > '9'))
        {
            return false;
        }
    }
    return true;
}

void WiFiConfigManager::reset()
{
    Serial.println("重置配置");
    preferences.clear();
}
