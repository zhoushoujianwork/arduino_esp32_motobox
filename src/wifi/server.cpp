#include "server.h"

#ifdef ENABLE_WIFI
WiFiConfigManager wifiManager;
#endif

void WiFiEvent(WiFiEvent_t event)
{
    // 电源倒计时的时候不处理
    if (powerManager.powerState == POWER_STATE_COUNTDOWN) {
        return;
    }
    switch (event)
    {
    case SYSTEM_EVENT_AP_START:
        Serial.println("AP模式已启动");
        break;
    case SYSTEM_EVENT_AP_STOP:
        Serial.println("AP模式已停止");
        break;
    case SYSTEM_EVENT_STA_START:
        Serial.println("WiFi客户端模式已启动");
        break;
    case SYSTEM_EVENT_STA_STOP:
        Serial.println("WiFi客户端模式已停止");
        break;
    case SYSTEM_EVENT_STA_CONNECTED:
        Serial.println("已连接到WiFi网络");
        device_state.wifiConnected = true;
        state_changes.wifi_changed = true;
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        Serial.println("WiFi连接已断开");
        device_state.wifiConnected = false;
        state_changes.wifi_changed = true;
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        Serial.printf("获取到IP地址: %s\n", WiFi.localIP().toString().c_str());
        // 更新设备状态
        device_state.wifiConnected = true;
        state_changes.wifi_changed = true;
        break;
    default:
        break;
    }
}

WiFiConfigManager::WiFiConfigManager() 
{
    // 注册WiFi事件回调
    WiFi.onEvent(WiFiEvent);
    // 配置WiFi modem sleep模式
    esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
    Serial.println("WiFi modem sleep已启用");
}

void WiFiConfigManager::enterAPMode()
{
    Serial.println("进入配网模式");
    if (isConfigMode) {
        Serial.println("已在配网模式，忽略重复进入请求");
        return;
    }
    
    server.stop();
    dnsServer.stop();
    delay(100);

    WiFi.disconnect(true, true);
    delay(200);

    WiFi.mode(WIFI_AP);
    delay(100);

    isConfigMode = true;

    setupAP();
    setupDNS();
    setupWebServer();
    Serial.println("WiFi配网模式已启动");
}

void WiFiConfigManager::setupAP()
{
    apSSID = String(APP_NAME) + "-" + String(device.get_device_id());
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
    else
    {
        Serial.println("DNS服务器启动失败");
    }
}

void WiFiConfigManager::setupWebServer()
{
    server.on("/", HTTP_GET, [this]() {
        server.send(200, "text/html", getConfigPage());
    });

    server.on("/configure", HTTP_POST, [this]() {
        handleConfigSubmit();
    });

    // 处理各种设备的 Captive Portal 检测
    server.on("/hotspot-detect.html", HTTP_GET, [this]() {
        server.send(200, "text/html", getConfigPage());
    });
    server.on("/generate_204", HTTP_GET, [this]() {
        server.send(200, "text/html", getConfigPage());
    });
    server.on("/redirect", HTTP_GET, [this]() {
        server.send(200, "text/html", getConfigPage());
    });
    server.on("/ncsi.txt", HTTP_GET, [this]() {
        server.send(200, "text/html", getConfigPage());
    });
    server.on("/fwlink", HTTP_GET, [this]() {
        server.send(200, "text/html", getConfigPage());
    });
    server.on("/connecttest.txt", HTTP_GET, [this]() {
        server.send(200, "text/html", getConfigPage());
    });
    server.on("/success.txt", HTTP_GET, [this]() {
        server.send(200, "text/html", getConfigPage());
    });

    // WiFi扫描接口
    server.on("/scan", HTTP_GET, [this]() {
        int n = WiFi.scanNetworks();
        DynamicJsonDocument doc(1024);
        JsonArray arr = doc.to<JsonArray>();
        for (int i = 0; i < n; ++i) {
            arr.add(WiFi.SSID(i));
        }
        String output;
        serializeJson(doc, output);
        server.send(200, "application/json", output);
    });

    // 获取已保存WiFi（单个）
    server.on("/saved_wifi", HTTP_GET, [this]() {
        String ssid, password;
        DynamicJsonDocument doc(256);
        JsonArray arr = doc.to<JsonArray>();
        if (PreferencesUtils::getWifi(ssid, password) && ssid.length() > 0) {
            JsonObject obj = arr.createNestedObject();
            obj["ssid"] = ssid;
            obj["password"] = password;
        }
        String output;
        serializeJson(arr, output);
        server.send(200, "application/json", output);
    });

    // 删除已保存WiFi
    server.on("/delete_wifi", HTTP_POST, [this]() {
        PreferencesUtils::clearWifi();
        // 返回空列表
        DynamicJsonDocument doc(256);
        JsonArray arr = doc.to<JsonArray>();
        String output;
        serializeJson(arr, output);
        server.send(200, "application/json", output);
    });

    // 退出配网模式
    server.on("/exit_config", HTTP_POST, [this]() {
        this->exitAPMode();
        server.send(200, "application/json", "{\"ok\":1}");
    });

    // 处理所有未定义的请求
    server.onNotFound([this]() {
        if (!isIp(server.hostHeader())) {
            server.sendHeader("Location", String("http://") + WiFi.softAPIP().toString(), true);
            server.send(302, "text/plain", "");
        } else {
            server.send(200, "text/html", getConfigPage());
        }
    });

    server.begin();
    Serial.println("配置服务器已启动");
}

void WiFiConfigManager::handleConfigSubmit()
{
    String newSSID = server.arg("ssid");
    String newPassword = server.arg("password");
    
    // 验证输入数据
    if (newSSID.isEmpty() || newSSID == "null") {
        server.send(400, "text/plain", "SSID不能为空");
        return;
    }
    
    Serial.printf("[WiFi] 保存新的WiFi配置: SSID=%s\n", newSSID.c_str());
    saveWiFiCredentials(newSSID, newPassword);
    server.send(200, "text/html", getSuccessPage());
    delay(2000);
    ESP.restart();
}

void WiFiConfigManager::handleClient()
{
    dnsServer.processNextRequest();
    server.handleClient();
}

String WiFiConfigManager::getConfigPage()
{
    return FPSTR(WIFI_CONFIG_PAGE_HTML);
}

void WiFiConfigManager::saveWiFiCredentials(const String &ssid, const String &password)
{
    PreferencesUtils::saveWifi(ssid, password);
}

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
            else return false;
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
    Serial.println("[WiFi] 执行重置...");
    WiFi.disconnect(true, true);
    PreferencesUtils::clearWifi();
    delay(100);
    ESP.restart();
}

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
                window.onload = function() {
                    var seconds = 3;
                    var countdownElement = document.getElementById('countdown');
                    var interval = setInterval(function() {
                        seconds--;
                        countdownElement.textContent = seconds + ' 秒后重启...';
                        if (seconds <= 0) {
                            clearInterval(interval);
                            // 尝试跳转到设备IP地址
                            var deviceIP = window.location.hostname;
                            window.location.href = 'http://' + deviceIP;
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
                <p>设备重启后，将自动跳转到设备页面。</p>
                <div id="countdown" class="countdown">3 秒后重启...</div>
            </div>
        </body>
        </html>
    )";
    return html;
}

void WiFiConfigManager::exitAPMode()
{
    if (!isConfigMode) {
        Serial.println("当前不在配网模式，无需退出");
        return;
    }
    
    Serial.println("退出配网模式");
    isConfigMode = false;
    
    server.stop();
    dnsServer.stop();
    
    WiFi.disconnect(true, false);
    delay(100);
    
    WiFi.mode(WIFI_OFF);
    delay(100);
}

bool WiFiConfigManager::isAPModeActive() const
{
    return WiFi.getMode() == WIFI_AP;
}

bool WiFiConfigManager::hasAPClient() const
{
    return WiFi.softAPgetStationNum() > 0;
}

void WiFiConfigManager::safeDisableWiFi()
{
    if (WiFi.getMode() != WIFI_OFF) {
        Serial.println("[WiFi] 正在断开WiFi...");
        WiFi.disconnect(true, true);
        delay(200);
        WiFi.mode(WIFI_OFF);
        delay(100);
        esp_wifi_deinit();
        Serial.println("[WiFi] WiFi驱动已释放");
    }
}

void WiFiConfigManager::loop()
{
    if (isConfigMode) {
        static unsigned long lastProcessTime = 0;
        unsigned long currentTime = millis();
        
        // 每10ms最多处理一次请求
        if (currentTime - lastProcessTime >= 10) {
            lastProcessTime = currentTime;
            
            // 处理DNS请求，设置超时
            dnsServer.processNextRequest();
            
            // 处理Web服务器请求，设置超时
            server.handleClient();
        }
    }
}

void WiFiConfigManager::setConfigMode(bool mode)
{
    isConfigMode = mode;
    if (mode) {
        // 启动AP模式
        startAP();
    } else {
        // 关闭AP模式
        stopAP();
    }
}

void WiFiConfigManager::startAP()
{
    setupAP();
    setupDNS();
    setupWebServer();
}

void WiFiConfigManager::stopAP()
{
    server.stop();
    dnsServer.stop();
}



