/*
 * Air780EG 模块测试程序
 * 用于验证Air780EG的基本功能
 */

#include "Air780EGModem.h"
#include "Air780EGMqtt.h"

// 测试配置
#define TEST_MQTT_SERVER "your-mqtt-server.com"
#define TEST_MQTT_PORT 1883
#define TEST_CLIENT_ID "air780eg-test"
#define TEST_USERNAME ""
#define TEST_PASSWORD ""

// 前置声明
void printModuleStatus();
void printGNSSStatus();
void publishTestMessage();
void handleSerialCommands();
void handleTestCommand(const String& command);
void printHelp();

// 全局变量
Air780EGMqtt* testMqtt = nullptr;
unsigned long lastStatusPrint = 0;
unsigned long lastGNSSPrint = 0;
unsigned long lastMQTTPublish = 0;

void air780eg_test_setup() {
    Serial.println("=== Air780EG 模块测试开始 ===");
    
    // 初始化模块
    air780eg_modem.setDebug(true);
    
    Serial.println("1. 初始化Air780EG模块...");
    if (!air780eg_modem.begin()) {
        Serial.println("❌ Air780EG初始化失败");
        return;
    }
    Serial.println("✅ Air780EG初始化成功");
    
    // 等待网络注册
    Serial.println("2. 等待网络注册...");
    int networkRetry = 0;
    while (!air780eg_modem.isNetworkReady() && networkRetry < 30) {
        Serial.print(".");
        delay(2000);
        networkRetry++;
    }
    
    if (air780eg_modem.isNetworkReady()) {
        Serial.println("\n✅ 网络注册成功");
        air780eg_modem.debugNetworkInfo();
    } else {
        Serial.println("\n❌ 网络注册失败");
        return;
    }
    
    // 测试GNSS功能
    Serial.println("3. 启用GNSS功能...");
    if (air780eg_modem.enableGNSS(true)) {
        Serial.println("✅ GNSS启用成功");
        air780eg_modem.setGNSSUpdateRate(2);  // 2Hz更新
    } else {
        Serial.println("❌ GNSS启用失败");
    }
    
    // 测试MQTT连接
    Serial.println("4. 测试MQTT连接...");
    testMqtt = new Air780EGMqtt(air780eg_modem);
    testMqtt->setDebug(true);
    
    if (testMqtt->begin()) {
        if (testMqtt->connect(TEST_MQTT_SERVER, TEST_MQTT_PORT, TEST_CLIENT_ID, TEST_USERNAME, TEST_PASSWORD)) {
            Serial.println("✅ MQTT连接成功");
            
            // 订阅测试主题
            testMqtt->subscribe("air780eg/test/command", 1);
            
            // 设置消息回调
            testMqtt->setCallback([](String topic, String payload) {
                Serial.println("📨 收到MQTT消息: " + topic + " -> " + payload);
                
                // 处理测试命令
                if (topic == "air780eg/test/command") {
                    handleTestCommand(payload);
                }
            });
        } else {
            Serial.println("❌ MQTT连接失败");
        }
    } else {
        Serial.println("❌ MQTT初始化失败");
    }
    
    Serial.println("=== Air780EG 测试初始化完成 ===\n");
}

void air780eg_test_loop() {
    unsigned long now = millis();
    
    // 每10秒打印一次状态信息
    if (now - lastStatusPrint > 10000) {
        lastStatusPrint = now;
        printModuleStatus();
    }
    
    // 每5秒打印一次GNSS信息
    if (now - lastGNSSPrint > 5000) {
        lastGNSSPrint = now;
        printGNSSStatus();
    }
    
    // MQTT消息处理
    if (testMqtt && testMqtt->isConnected()) {
        testMqtt->loop();
        
        // 每30秒发布一次测试消息
        if (now - lastMQTTPublish > 30000) {
            lastMQTTPublish = now;
            publishTestMessage();
        }
    }
    
    // 检查串口命令
    handleSerialCommands();
}

void printModuleStatus() {
    Serial.println("=== 模块状态 ===");
    Serial.println("网络状态: " + String(air780eg_modem.isNetworkReady() ? "已连接" : "未连接"));
    Serial.println("信号强度: " + String(air780eg_modem.getCSQ()));
    Serial.println("运营商: " + air780eg_modem.getCarrierName());
    Serial.println("网络类型: " + air780eg_modem.getNetworkType());
    Serial.println("本地IP: " + air780eg_modem.getLocalIP());
    
    if (testMqtt) {
        Serial.println("MQTT状态: " + String(testMqtt->isConnected() ? "已连接" : "未连接"));
    }
    Serial.println();
}

void printGNSSStatus() {
    if (air780eg_modem.updateGNSSData()) {
        GNSSData gnss = air780eg_modem.getGNSSData();
        
        Serial.println("=== GNSS状态 ===");
        Serial.println("定位状态: " + String(gnss.valid ? "有效" : "无效"));
        
        if (gnss.valid) {
            Serial.printf("纬度: %.6f°\n", gnss.latitude);
            Serial.printf("经度: %.6f°\n", gnss.longitude);
            Serial.printf("海拔: %.2f米\n", gnss.altitude);
            Serial.printf("速度: %.2f km/h\n", gnss.speed);
            Serial.printf("方向: %.2f°\n", gnss.course);
            Serial.printf("卫星数: %d\n", gnss.satellites);
            Serial.printf("HDOP: %.2f\n", gnss.hdop);
            Serial.println("时间: " + gnss.timestamp);
            Serial.println("定位模式: " + gnss.fix_mode);
        }
        Serial.println();
    }
}

void publishTestMessage() {
    if (!testMqtt || !testMqtt->isConnected()) return;
    
    // 发布状态消息
    String statusJson = "{";
    statusJson += "\"timestamp\":" + String(millis()) + ",";
    statusJson += "\"network\":\"" + String(air780eg_modem.isNetworkReady() ? "connected" : "disconnected") + "\",";
    statusJson += "\"signal\":" + String(air780eg_modem.getCSQ()) + ",";
    statusJson += "\"carrier\":\"" + air780eg_modem.getCarrierName() + "\",";
    statusJson += "\"ip\":\"" + air780eg_modem.getLocalIP() + "\"";
    
    // 添加GNSS数据
    if (air780eg_modem.isGNSSDataValid()) {
        GNSSData gnss = air780eg_modem.getGNSSData();
        statusJson += ",\"gnss\":{";
        statusJson += "\"valid\":" + String(gnss.valid ? "true" : "false") + ",";
        statusJson += "\"lat\":" + String(gnss.latitude, 6) + ",";
        statusJson += "\"lon\":" + String(gnss.longitude, 6) + ",";
        statusJson += "\"alt\":" + String(gnss.altitude, 2) + ",";
        statusJson += "\"speed\":" + String(gnss.speed, 2) + ",";
        statusJson += "\"course\":" + String(gnss.course, 2) + ",";
        statusJson += "\"satellites\":" + String(gnss.satellites) + ",";
        statusJson += "\"hdop\":" + String(gnss.hdop, 2);
        statusJson += "}";
    }
    
    statusJson += "}";
    
    testMqtt->publish("air780eg/test/status", statusJson, 1);
    Serial.println("📤 发布状态消息: " + statusJson.substring(0, 100) + "...");
}

void handleTestCommand(const String& command) {
    Serial.println("🔧 处理测试命令: " + command);
    
    if (command == "info") {
        air780eg_modem.debugNetworkInfo();
        air780eg_modem.debugGNSSInfo();
    }
    else if (command == "gnss_on") {
        air780eg_modem.enableGNSS(true);
        Serial.println("GNSS已启用");
    }
    else if (command == "gnss_off") {
        air780eg_modem.enableGNSS(false);
        Serial.println("GNSS已禁用");
    }
    else if (command == "reset") {
        Serial.println("重置模块...");
        air780eg_modem.reset();
    }
    else if (command.startsWith("rate_")) {
        int rate = command.substring(5).toInt();
        if (air780eg_modem.setGNSSUpdateRate(rate)) {
            Serial.println("GNSS更新频率设置为: " + String(rate) + "Hz");
        } else {
            Serial.println("GNSS更新频率设置失败");
        }
    }
    else {
        Serial.println("未知命令: " + command);
    }
}

void handleSerialCommands() {
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();
        
        if (cmd == "help") {
            printHelp();
        }
        else if (cmd == "status") {
            printModuleStatus();
        }
        else if (cmd == "gnss") {
            printGNSSStatus();
        }
        else if (cmd == "info") {
            air780eg_modem.debugNetworkInfo();
            air780eg_modem.debugGNSSInfo();
        }
        else if (cmd == "test") {
            publishTestMessage();
        }
        else if (cmd.startsWith("at ")) {
            String atCmd = cmd.substring(3);
            String response = air780eg_modem.sendATWithResponse(atCmd, 5000);
            Serial.println("AT响应: " + response);
        }
        else {
            Serial.println("未知命令，输入 'help' 查看帮助");
        }
    }
}

void printHelp() {
    Serial.println("=== Air780EG 测试命令 ===");
    Serial.println("help    - 显示帮助信息");
    Serial.println("status  - 显示模块状态");
    Serial.println("gnss    - 显示GNSS状态");
    Serial.println("info    - 显示详细信息");
    Serial.println("test    - 发布测试消息");
    Serial.println("at <cmd> - 发送AT命令");
    Serial.println("========================");
}
