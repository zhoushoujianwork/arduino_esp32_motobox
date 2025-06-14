#include "WifiManager.h"
#include "power/PowerManager.h"
#include "wifi/wifi_config_page.h"
#include "utils/PreferencesUtils.h"
#include "nvs_flash.h"

// WiFi事件回调函数
unsigned long wifiConnectedTime = 0;
bool needCheckInternet = false;

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
    case SYSTEM_EVENT_STA_START:
        Serial.println("STA WiFi已开始连接");
        break;
    case SYSTEM_EVENT_STA_STOP:
        Serial.println("STA WiFi已停止连接");
        break;
    case SYSTEM_EVENT_STA_CONNECTED:
        Serial.println("STA WiFi已连接");
        device_state.wifiConnected = true;
        wifiConnectedTime = millis();
        needCheckInternet = true;
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        Serial.println("STA WiFi已断开");
        device_state.wifiConnected = false;
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        Serial.printf("STA 获取到IP地址: %s\n", WiFi.localIP().toString().c_str());
        break;
    case SYSTEM_EVENT_AP_START:
        Serial.println("AP模式已启动");
        device_state.wifiConnected = false;
        break;
    case SYSTEM_EVENT_AP_STOP:
        Serial.println("AP模式已停止");
        break;
    default:
        break;
    }
}



// 在类成员变量中添加
unsigned long lastConnectAttempt = 0;



WiFiConfigManager::WiFiConfigManager()
{
    
}

void WiFiConfigManager::begin()
{
    
    Serial.println("[WiFi] 初始化开始");
    
    // 注册WiFi事件回调
    WiFi.onEvent(WiFiEvent);

    // 配置WiFi modem sleep模式
    esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
    Serial.println("WiFi modem sleep已启用");

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
    // 如果在进入倒计时休眠状态直接条幅
    if (powerManager.powerState == POWER_STATE_COUNTDOWN) {
        return;
    }
    
    if (isConfigMode)
    {
        handleClient();
    }
    else
    {
        if (WiFi.status() != WL_CONNECTED)
        {
            device_state.wifiConnected = false;

            // 每5秒重试一次
            if (millis() - lastConnectAttempt > 5000)
            {
                lastConnectAttempt = millis();
                if (tryConnectWithSavedCredentials())
                {
                    Serial.println("\nWiFi连接成功!");
                    device_state.wifiConnected = true;
                }
            }
        }
    }

    if (isConfigMode && WiFi.getMode() != WIFI_AP) {
        WiFi.mode(WIFI_AP);
        Serial.println("WiFi模式已切换为AP");
    }
}

// 修改 tryConnectWithSavedCredentials
// 特性：如果没有已保存的WiFi，返回false，通知外部进入配网模式
bool WiFiConfigManager::tryConnectWithSavedCredentials()
{
    String wifiListStr = PreferencesUtils::loadString(PreferencesUtils::NS_WIFI, PreferencesUtils::KEY_WIFI_LIST, "[]");
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, wifiListStr);
    JsonArray arr = doc.as<JsonArray>();

    if (arr.size() == 0)
    {
        Serial.println("未找到已保存的WiFi配置");
        enterConfigMode();
        return false;
    }

    for (JsonObject obj : arr)
    {
        if (isConfigMode) {
            Serial.println("已在配网模式，忽略连接请求");
            return false;
        }
        String ssid = obj["ssid"].as<String>();
        String password = obj["password"].as<String>();
        Serial.printf("尝试连接WiFi: %s, 密码: %s\n", ssid.c_str(), password.c_str());
        if (tryConnectSingle(ssid, password))
        {
            Serial.printf("\nWiFi连接成功: %s\n", ssid.c_str());
            return true;
        }
        Serial.println("连接失败，继续尝试下一个WiFi");
    }
    Serial.println("所有已保存WiFi均连接失败");
    return false;
}

void WiFiConfigManager::enterConfigMode()
{
    if (isConfigMode) {
        Serial.println("已在配网模式，忽略重复进入请求");
        return;
    }
    // 关闭 WebServer 和 DNS Server
    server.stop();
    dnsServer.stop();
    delay(100);

    // 彻底断开STA并忘记所有连接
    WiFi.disconnect(true, true); // 第二个参数true表示"忘记所有已保存的STA配置"
    delay(200);

    // 强制只做AP
    WiFi.mode(WIFI_AP);
    delay(100);

    isConfigMode = true;
    device_state.wifiConnected = false;

    setupAP();
    setupDNS();
    setupWebServer();
}

void WiFiConfigManager::setupAP()
{
    WiFi.disconnect(true);
    delay(100);

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

    // 新增：WiFi扫描接口，返回JSON热点列表
    server.on("/scan", HTTP_GET, [this]()
              {
        int n = WiFi.scanNetworks();
        DynamicJsonDocument doc(1024);
        JsonArray arr = doc.to<JsonArray>();
        for (int i = 0; i < n; ++i) {
            arr.add(WiFi.SSID(i));
        }
        String output;
        serializeJson(doc, output);
        server.send(200, "application/json", output); });

    // 新增：获取已保存WiFi列表接口
    server.on("/saved_wifi", HTTP_GET, [this]()
              {
        String wifiListStr = PreferencesUtils::loadString(PreferencesUtils::NS_WIFI, PreferencesUtils::KEY_WIFI_LIST, "[]");
        Serial.printf("[Web] 查询已保存WiFi: %s\n", wifiListStr.c_str());
        server.send(200, "application/json", wifiListStr); });

    // 新增：删除已保存WiFi接口
    server.on("/delete_wifi", HTTP_POST, [this]()
              {
        String ssid = server.arg("ssid");
        Serial.printf("[Web] 请求删除WiFi: %s\n", ssid.c_str());
        String wifiListStr = PreferencesUtils::loadString(PreferencesUtils::NS_WIFI, PreferencesUtils::KEY_WIFI_LIST, "[]");
        DynamicJsonDocument doc(1024);
        DeserializationError err = deserializeJson(doc, wifiListStr);
        if (err) {
            Serial.printf("[Web] 解析WiFi列表失败: %s\n", err.c_str());
            server.send(500, "application/json", "{\"error\":\"json parse error\"}");
            return;
        }
        JsonArray arr = doc.as<JsonArray>();
        bool deleted = false;
        for (size_t i = 0; i < arr.size(); ++i) {
            if (arr[i]["ssid"] == ssid) {
                arr.remove(i);
                deleted = true;
                break;
            }
        }
        String output;
        serializeJson(doc, output);
        PreferencesUtils::saveString(PreferencesUtils::NS_WIFI, PreferencesUtils::KEY_WIFI_LIST, output);
        Serial.printf("[Web] 删除结果: %s, 新列表: %s\n", deleted ? "已删除" : "未找到", output.c_str());
        server.send(200, "application/json", output); });

    // 新增：退出配网模式，切换为STA并尝试连接已保存WiFi
    server.on("/exit_config", HTTP_POST, [this]() {
        this->exitConfigMode();
        server.send(200, "application/json", "{\"ok\":1}");
    });

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
    // 直接保存，不做连接校验
    saveWiFiCredentials(newSSID, newPassword);
    String html = R"(
        <!DOCTYPE html>
        <html>
        <head>
            <meta charset='utf-8'>
            <meta name='viewport' content='width=device-width, initial-scale=1'>
            <title>WiFi配置成功</title>
            <style>
                body { font-family: Arial, sans-serif; background: #f5f5f5; text-align: center; padding: 40px; }
                .container { background: #fff; border-radius: 10px; box-shadow: 0 2px 5px rgba(0,0,0,0.1); padding: 30px; max-width: 400px; margin: 0 auto; }
                h1 { color: #007bff; margin-bottom: 20px; }
                p { color: #333; font-size: 16px; margin-bottom: 20px; }
            </style>
        </head>
        <body>
            <div class='container'>
                <h1>WiFi配置成功！</h1>
                <p>设备即将重启，并自动尝试连接您填写的WiFi网络。</p>
                <p>本页面即将失联，请关闭本页面。</p>
            </div>
        </body>
        </html>
    )";
    server.send(200, "text/html", html);
    delay(2000); // 给用户2秒时间看到提示
    WiFi.softAPdisconnect(true); // 关闭AP
    delay(500);
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
    // 读取已保存热点
    String wifiListStr = PreferencesUtils::loadString(PreferencesUtils::NS_WIFI, PreferencesUtils::KEY_WIFI_LIST, "[]");
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, wifiListStr);
    JsonArray arr = doc.as<JsonArray>();
    // 检查是否已存在
    bool found = false;
    for (JsonObject obj : arr)
    {
        if (obj["ssid"] == ssid)
        {
            obj["password"] = password;
            found = true;
            break;
        }
    }
    if (!found)
    {
        JsonObject newObj = arr.createNestedObject();
        newObj["ssid"] = ssid;
        newObj["password"] = password;
    }
    String output;
    serializeJson(doc, output);
    PreferencesUtils::saveString(PreferencesUtils::NS_WIFI, PreferencesUtils::KEY_WIFI_LIST, output);
    Serial.printf("WiFi凭据保存成功: %s, %s\n", ssid.c_str(), password.c_str());
}

// 检查MQTT服务器连通性（使用config.h配置）
// 特性：仅检测TCP连接，不做完整MQTT握手，地址端口由config.h宏定义
bool WiFiConfigManager::checkInternetConnection()
{
    WiFiClient client;
    Serial.printf("检测MQTT服务器连通性: %s:%d\n", MQTT_BROKER, MQTT_PORT);

    // 尝试连接
    if (!client.connect(MQTT_BROKER, MQTT_PORT))
    {
        Serial.println("MQTT服务器连接失败！");
        return false;
    }

    Serial.println("MQTT服务器连接成功！");
    client.stop();
    return true;
}

// 添加新的辅助方法来检查是否是IP地址
bool WiFiConfigManager::isIp(String str)
{
    for (int i = 0; i < 4; i++)
    {
        int nOctet = 0;
        int nLen = 0;

        while (nLen < str.length())
        {
            char c = str[nLen];

            if (c == '.')
            {
                if (i < 3)
                    break;
                else
                    return false;
            }
            else if (c >= '0' && c <= '9')
            {
                nOctet = nOctet * 10 + (c - '0');
                if (nOctet > 255)
                    return false;
            }
            else
            {
                return false;
            }

            nLen++;
        }

        if (nOctet > 255 || nLen == 0)
            return false;
        if (i < 3 && str[nLen] != '.')
            return false;

        str = str.substring(nLen + 1);
    }

    return str.length() == 0;
}

void WiFiConfigManager::reset()
{
    // 重置WiFi配置并重启设备
    Serial.println("[BLE] 执行 WiFi 重置...");
    WiFi.disconnect(true, true);
    preferences.clear();
    delay(100);
    ESP.restart();
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

// 新增：尝试连接单个热点
// 特性：用于表单提交时验证密码
bool WiFiConfigManager::tryConnectSingle(const String &ssid, const String &password)
{
    WiFi.disconnect(true);
    delay(500);
    WiFi.mode(WIFI_STA);
    delay(100);
    WiFi.begin(ssid.c_str(), password.c_str());
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED)
    {
        if (millis() - start > 10000)
            return false;
        delay(300);
    }
    return WiFi.status() == WL_CONNECTED;
}

// 新增：退出配网模式，切换为STA并尝试连接已保存WiFi
void WiFiConfigManager::exitConfigMode() {
    if (!isConfigMode) {
        Serial.println("当前不在配网模式，无需退出");
        return;
    }
    Serial.println("退出配网模式，切换为STA");
    isConfigMode = false;
    server.stop();
    dnsServer.stop();
    WiFi.disconnect(true, false); // 断开AP
    delay(100);
    WiFi.mode(WIFI_STA);
    delay(100);
    tryConnectWithSavedCredentials();
}

bool WiFiConfigManager::isAPModeActive() const {
    wifi_mode_t mode = WiFi.getMode();
    return (mode == WIFI_AP || mode == WIFI_AP_STA);
}

bool WiFiConfigManager::hasAPClient() const {
    return WiFi.softAPgetStationNum() > 0;
}

void WiFiConfigManager::safeDisableWiFi() {
    if (WiFi.getMode() != WIFI_OFF) {
        Serial.println("[电源管理] 正在断开WiFi...");
        WiFi.disconnect(true, true); // 断开并忘记配置
        unsigned long start = millis();
        // 等待WiFi断开，最多2秒
        while (WiFi.status() == WL_CONNECTED && millis() - start < 2000) {
            delay(10);
        }
        Serial.println("[电源管理] WiFi已断开，准备关闭WiFi模块...");
        WiFi.mode(WIFI_OFF);
        delay(100); // 等待模式切换
        esp_wifi_deinit();
        Serial.println("[电源管理] WiFi驱动已释放");
    }
}
