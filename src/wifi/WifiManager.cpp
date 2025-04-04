#include "WifiManager.h"

WiFiConfigManager wifiManager;

void WiFiConfigManager::begin()
{
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
}

// 阻塞的
void WiFiConfigManager::loop()
{
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
}
bool WiFiConfigManager::tryConnectWithSavedCredentials()
{
    String ssid = preferences.getString("ssid", "");
    String password = preferences.getString("password", "");
    
    if (ssid.isEmpty())
    {
        Serial.println("未找到已保存的WiFi配置");
        enterConfigMode();
        return false;
    }

    // 增加WiFi连接的健壮性
    WiFi.disconnect(true);  // 确保断开之前的连接
    delay(1000);  // 增加延迟时间，确保完全断开

    // 重置WiFi状态
    WiFi.mode(WIFI_OFF);
    delay(100);
    WiFi.mode(WIFI_STA);
    delay(100);

    // 设置更保守的连接参数
    WiFi.setAutoReconnect(false);  // 禁用自动重连，由我们手动控制
    WiFi.setSleep(false);  // 禁用省电模式，提高稳定性
    
    Serial.printf("尝试连接WiFi: %s\n", ssid.c_str());
    Serial.printf("尝试连接WiFi密码: %s\n", password.c_str());

    // 使用临时变量存储凭据，避免直接使用String对象
    const char* ssid_ptr = ssid.c_str();
    const char* password_ptr = password.c_str();
    
    if (!ssid_ptr || !password_ptr) {
        Serial.println("WiFi凭据无效");
        return false;
    }

    // 开始连接
    WiFi.begin(ssid_ptr, password_ptr);
    
    // 使用更安全的连接等待逻辑
    unsigned long startAttemptTime = millis();
    const unsigned long CONNECT_TIMEOUT = 20000; // 20秒超时
    
    while (WiFi.status() != WL_CONNECTED) 
    {
        if (millis() - startAttemptTime > CONNECT_TIMEOUT) {
            Serial.println("WiFi连接超时");
            WiFi.disconnect(true);
            return false;
        }

        // 使用switch语句处理不同的连接状态
        switch(WiFi.status()) {
            case WL_DISCONNECTED:
                Serial.print(".");
                break;
            case WL_CONNECT_FAILED:
                Serial.println("\n连接失败，可能是密码错误");
                WiFi.disconnect(true);
                return false;
            case WL_NO_SSID_AVAIL:
                Serial.println("\n未找到指定的WiFi网络");
                return false;
            case WL_IDLE_STATUS:
                Serial.print("i");
                break;
            default:
                Serial.print("?");
                break;
        }

        delay(500);  // 增加延迟，减少CPU负载
    }

    // 连接成功后的处理
    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("\nWiFi连接成功，IP地址: %s\n", WiFi.localIP().toString().c_str());
        
        // 验证互联网连接
        if (!checkInternetConnection()) {
            Serial.println("互联网连接验证失败");
            WiFi.disconnect(true);
            return false;
        }

        device.set_wifi_connected(true);
        return true;
    }

    return false;
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
    // 设置 DNS 服务器响应所有域名查询
    dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    if (dnsServer.start(53, "*", WiFi.softAPIP()))
    {
        Serial.println("DNS服务器启动成功");
        Serial.printf("DNS服务器IP: %s\n", WiFi.softAPIP().toString().c_str());
    }
    else
    {
        Serial.println("DNS服务器启动失败");
    }
}

void WiFiConfigManager::setupWebServer()
{
    // 处理根路径请求
    server.on("/", HTTP_GET, [this]()
              { server.send(200, "text/html", getConfigPage()); });

    server.on("/configure", HTTP_POST, [this]()
              { handleConfigSubmit(); });

    // 处理 Apple 设备的 Captive Portal 检测
    server.on("/hotspot-detect.html", HTTP_GET, [this]()
              { server.send(200, "text/html", getConfigPage()); });

    // 处理 Android 设备的 Captive Portal 检测
    server.on("/generate_204", HTTP_GET, [this]()
              { server.send(200, "text/html", getConfigPage()); });
    
    server.on("/redirect", HTTP_GET, [this]()
              { server.send(200, "text/html", getConfigPage()); });

    // 处理 Windows 设备的 Captive Portal 检测
    server.on("/ncsi.txt", HTTP_GET, [this]()
              { server.send(200, "text/html", getConfigPage()); });

    server.on("/fwlink", HTTP_GET, [this]()
              { server.send(200, "text/html", getConfigPage()); });

    server.on("/connecttest.txt", HTTP_GET, [this]()
              { server.send(200, "text/html", getConfigPage()); });

    server.on("/success.txt", HTTP_GET, [this]()
              { server.send(200, "text/html", getConfigPage()); });

    // 处理所有未定义的请求
    server.onNotFound([this]()
                     {
        if (!isIp(server.hostHeader()))
        {
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
    server.send(200, "text/html", getSuccessPage());  // 使用新的成功页面
    delay(3000);  // 延长重启前的时间，让用户看到动画
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
    for (int i = 0; i < 4; i++) {
        int nOctet = 0;
        int nLen = 0;
        
        while (nLen < str.length()) {
            char c = str[nLen];
            
            if (c == '.') {
                if (i < 3) break;
                else return false;
            }
            else if (c >= '0' && c <= '9') {
                nOctet = nOctet * 10 + (c - '0');
                if (nOctet > 255) return false;
            }
            else {
                return false;
            }
            
            nLen++;
        }
        
        if (nOctet > 255 || nLen == 0) return false;
        if (i < 3 && str[nLen] != '.') return false;
        
        str = str.substring(nLen + 1);
    }
    
    return str.length() == 0;
}

void WiFiConfigManager::reset()
{
    Serial.println("重置配置");
    preferences.clear();
}

// 添加一个新函数，用于返回成功页面的HTML
String WiFiConfigManager::getSuccessPage()
{
    String html = R"(
        <!DOCTYPE html>
        <html>
        <head>
            <meta charset='utf-8'>
            <meta name='viewport' content='width=device-width, initial-scale=1'>
            <title>MotoBox - 配置成功</title>
            <style>
                body { 
                    font-family: Arial, sans-serif;
                    padding: 20px;
                    background-color: #f5f5f5;
                    max-width: 600px;
                    margin: 0 auto;
                    text-align: center;
                }
                .container {
                    background: white;
                    padding: 30px;
                    border-radius: 10px;
                    box-shadow: 0 2px 5px rgba(0,0,0,0.1);
                    margin-top: 50px;
                }
                h1 {
                    color: #007bff;
                    font-size: 24px;
                    margin-bottom: 20px;
                }
                p {
                    color: #333;
                    font-size: 16px;
                    margin-bottom: 30px;
                }
                .loader {
                    border: 8px solid #f3f3f3;
                    border-radius: 50%;
                    border-top: 8px solid #007bff;
                    width: 60px;
                    height: 60px;
                    margin: 0 auto 20px;
                    animation: spin 2s linear infinite;
                }
                @keyframes spin {
                    0% { transform: rotate(0deg); }
                    100% { transform: rotate(360deg); }
                }
                .countdown {
                    font-size: 18px;
                    font-weight: bold;
                    color: #333;
                    margin-top: 15px;
                }
            </style>
            <script>
                // 倒计时脚本
                window.onload = function() {
                    var seconds = 3;
                    var countdownElement = document.getElementById('countdown');
                    
                    var interval = setInterval(function() {
                        seconds--;
                        countdownElement.textContent = seconds + ' 秒后重启...';
                        
                        if (seconds <= 0) {
                            clearInterval(interval);
                        }
                    }, 1000);
                };
            </script>
        </head>
        <body>
            <div class="container">
                <h1>WiFi配置成功!</h1>
                <div class="loader"></div>
                <p>MotoBox即将重新启动，连接到您的WiFi网络。</p>
                <p>设备重启后，请关闭此页面。</p>
                <div id="countdown" class="countdown">3 秒后重启...</div>
            </div>
        </body>
        </html>
    )";
    return html;
}
