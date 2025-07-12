// Compass 组件使用示例代码片段
// 可以将这些代码集成到 main.cpp 或其他需要使用compass的地方

#ifdef ENABLE_COMPASS
#include "compass/Compass.h"
#include "compass/compass_test.h"

// 在setup()函数中初始化compass
void setupCompass() {
    Serial.println("初始化罗盘...");
    
    if (compass.begin()) {
        Serial.println("✓ 罗盘初始化成功");
        
        // 设置磁偏角（根据地理位置调整）
        compass.setDeclination(-6.5);  // 中国大部分地区约为-6.5度
        
        // 可选：设置已知的校准参数
        // compass.setCalibration(-123, 456, -789, 1.05, 0.95, 1.02);
        
        // 启用调试输出
        compass.setDebug(true);
        
    } else {
        Serial.println("✗ 罗盘初始化失败");
    }
}

// 在loop()函数中处理compass数据
void handleCompass() {
    // 更新罗盘数据
    compass.loop();
    
    // 检查数据有效性并处理
    if (compass_data.isValid) {
        // 方法1：直接使用全局数据
        processCompassData();
        
        // 方法2：使用类方法获取数据
        processCompassWithMethods();
        
        // 监测方向变化
        monitorCompassChanges();
    }
}

// 处理罗盘数据的示例函数
void processCompassData() {
    static unsigned long lastProcessTime = 0;
    
    // 每2秒处理一次
    if (millis() - lastProcessTime < 2000) {
        return;
    }
    lastProcessTime = millis();
    
    Serial.printf("[罗盘数据] 航向: %.1f° (%s, %s)\n", 
        compass_data.heading,
        compass_data.directionStr,
        compass_data.directionCN);
    
    // 根据方向执行不同操作
    switch (compass_data.direction) {
        case NORTH:
            Serial.println("→ 正在朝北方向行驶");
            // 可以在这里添加北向特定的逻辑
            break;
            
        case EAST:
            Serial.println("→ 正在朝东方向行驶");
            break;
            
        case SOUTH:
            Serial.println("→ 正在朝南方向行驶");
            break;
            
        case WEST:
            Serial.println("→ 正在朝西方向行驶");
            break;
            
        default:
            Serial.printf("→ 正在朝%s方向行驶\n", compass_data.directionCN);
            break;
    }
    
    // 精确角度判断示例
    if (compass_data.heading >= 350 || compass_data.heading <= 10) {
        Serial.println("  [精确] 几乎正北方向");
    } else if (compass_data.heading >= 80 && compass_data.heading <= 100) {
        Serial.println("  [精确] 几乎正东方向");
    }
}

// 使用类方法获取数据的示例
void processCompassWithMethods() {
    static unsigned long lastMethodTime = 0;
    
    // 每3秒处理一次
    if (millis() - lastMethodTime < 3000) {
        return;
    }
    lastMethodTime = millis();
    
    if (!compass.isDataValid()) {
        Serial.println("[方法调用] 罗盘数据无效");
        return;
    }
    
    float heading = compass.getHeading();
    float headingRad = compass.getHeadingRadians();
    CompassDirection dir = compass.getCurrentDirection();
    const char* dirStr = compass.getCurrentDirectionStr();
    const char* dirName = compass.getCurrentDirectionName();
    const char* dirCN = compass.getCurrentDirectionCN();
    
    Serial.printf("[方法调用] 航向: %.1f° (%.3f rad), 方向: %s (%s, %s)\n",
        heading, headingRad, dirStr, dirName, dirCN);
}

// 串口命令处理示例
void handleCompassCommands(String command) {
    command.toLowerCase();
    
    if (command == "compass_status") {
        // 显示罗盘状态
        Serial.println("\n=== 罗盘状态 ===");
        Serial.printf("初始化状态: %s\n", compass.isInitialized() ? "已初始化" : "未初始化");
        Serial.printf("数据有效性: %s\n", compass.isDataValid() ? "有效" : "无效");
        Serial.printf("磁偏角: %.2f°\n", compass.getDeclination());
        
        if (compass_data.isValid) {
            Serial.printf("当前航向: %.2f°\n", compass_data.heading);
            Serial.printf("当前方向: %s (%s)\n", compass_data.directionStr, compass_data.directionCN);
            Serial.printf("磁场强度: X=%.1f, Y=%.1f, Z=%.1f\n", 
                compass_data.x, compass_data.y, compass_data.z);
        }
        Serial.println("================\n");
        
    } else if (command == "compass_test") {
        // 运行测试函数
        testCompassFunctions();
        
    } else if (command == "compass_calibrate") {
        // 启动校准
        compassCalibrationHelper();
        
    } else if (command == "compass_reset") {
        // 重置罗盘
        compass.reset();
        Serial.println("罗盘已重置");
        
    } else if (command.startsWith("compass_declination ")) {
        // 设置磁偏角
        float declination = command.substring(20).toFloat();
        compass.setDeclination(declination);
        Serial.printf("磁偏角已设置为: %.2f°\n", declination);
    }
}

// 数据记录示例（可用于SD卡存储）
String getCompassDataForLogging() {
    if (!compass_data.isValid) {
        return "";
    }
    
    // 创建CSV格式的数据行
    String logData = String(compass_data.timestamp) + "," +
                    String(compass_data.heading, 2) + "," +
                    String(compass_data.direction) + "," +
                    String(compass_data.directionStr) + "," +
                    String(compass_data.x, 1) + "," +
                    String(compass_data.y, 1) + "," +
                    String(compass_data.z, 1);
    
    return logData;
}

// JSON格式数据输出示例
String getCompassDataJSON() {
    if (!compass_data.isValid) {
        return "{}";
    }
    
    String json = "{";
    json += "\"timestamp\":" + String(compass_data.timestamp) + ",";
    json += "\"heading\":" + String(compass_data.heading, 2) + ",";
    json += "\"heading_rad\":" + String(compass_data.headingRadians, 4) + ",";
    json += "\"direction\":" + String(compass_data.direction) + ",";
    json += "\"direction_str\":\"" + String(compass_data.directionStr) + "\",";
    json += "\"direction_name\":\"" + String(compass_data.directionName) + "\",";
    json += "\"direction_cn\":\"" + String(compass_data.directionCN) + "\",";
    json += "\"magnetic_field\":{";
    json += "\"x\":" + String(compass_data.x, 1) + ",";
    json += "\"y\":" + String(compass_data.y, 1) + ",";
    json += "\"z\":" + String(compass_data.z, 1);
    json += "},";
    json += "\"valid\":" + String(compass_data.isValid ? "true" : "false");
    json += "}";
    
    return json;
}

#endif // ENABLE_COMPASS
