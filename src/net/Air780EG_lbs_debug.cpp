/*
 * Air780EG LBS 调试工具
 * 用于测试和调试 Air780EG 的 LBS 基站定位功能
 */

#include "config.h"

#ifdef USE_AIR780EG_GSM

#include "Air780EGModem.h"
#include "../gps/LBSData.h"

extern Air780EGModem air780eg_modem;

void debugAir780EGLbs(Air780EGModem* modem) {
    if (!modem) {
        Serial.println("❌ Air780EG Modem 指针为空");
        return;
    }
    
    Serial.println("=== Air780EG LBS 调试工具 ===");
    Serial.println();
    
    // 1. 检查基础状态
    Serial.println("1. 基础状态检查:");
    Serial.printf("   模块就绪: %s\n", modem->isReady() ? "✅ 是" : "❌ 否");
    Serial.printf("   网络就绪: %s\n", modem->isNetworkReady() ? "✅ 是" : "❌ 否");
    Serial.printf("   LBS启用: %s\n", modem->isLBSEnabled() ? "✅ 是" : "❌ 否");
    Serial.printf("   LBS加载中: %s\n", modem->isLBSLoading() ? "⏳ 是" : "❌ 否");
    Serial.println();
    
    if (!modem->isReady()) {
        Serial.println("❌ 模块未就绪，无法进行LBS测试");
        return;
    }
    
    if (!modem->isNetworkReady()) {
        Serial.println("❌ 网络未就绪，无法进行LBS测试");
        return;
    }
    
    // 2. 启用LBS功能
    Serial.println("2. 启用LBS功能:");
    if (!modem->isLBSEnabled()) {
        Serial.println("   正在启用LBS...");
        if (modem->enableLBS(true)) {
            Serial.println("   ✅ LBS启用成功");
        } else {
            Serial.println("   ❌ LBS启用失败");
            return;
        }
    } else {
        Serial.println("   ✅ LBS已启用");
    }
    Serial.println();
    
    // 3. 显示当前LBS数据
    Serial.println("3. 当前LBS数据:");
    lbs_data_t lbsData = modem->getLBSData();
    if (lbsData.valid) {
        Serial.printf("   纬度: %.6f\n", lbsData.latitude);
        Serial.printf("   经度: %.6f\n", lbsData.longitude);
        Serial.printf("   半径: %d 米\n", lbsData.radius);
        Serial.printf("   状态: %d\n", (int)lbsData.state);
        Serial.printf("   时间戳: %lu\n", lbsData.timestamp);
        Serial.printf("   数据年龄: %lu 秒\n", (millis() - lbsData.timestamp) / 1000);
    } else {
        Serial.println("   ❌ 无有效LBS数据");
    }
    Serial.println();
    
    // 4. 执行LBS定位测试
    Serial.println("4. 执行LBS定位测试:");
    Serial.println("   正在请求LBS定位...");
    Serial.println("   ⏳ 请等待（最多35秒）...");
    
    unsigned long startTime = millis();
    bool result = modem->updateLBSData();
    unsigned long endTime = millis();
    
    Serial.printf("   请求耗时: %lu 毫秒\n", endTime - startTime);
    
    if (result) {
        Serial.println("   ✅ LBS定位成功");
        
        // 显示更新后的数据
        lbs_data_t newLbsData = modem->getLBSData();
        Serial.println("   更新后的LBS数据:");
        Serial.printf("     纬度: %.6f\n", newLbsData.latitude);
        Serial.printf("     经度: %.6f\n", newLbsData.longitude);
        Serial.printf("     半径: %d 米\n", newLbsData.radius);
        Serial.printf("     状态: %d\n", (int)newLbsData.state);
        Serial.printf("     时间戳: %lu\n", newLbsData.timestamp);
        
        // 计算与之前数据的差异
        if (lbsData.valid) {
            float latDiff = abs(newLbsData.latitude - lbsData.latitude);
            float lonDiff = abs(newLbsData.longitude - lbsData.longitude);
            Serial.printf("     与上次差异: 纬度 %.6f, 经度 %.6f\n", latDiff, lonDiff);
        }
    } else {
        Serial.println("   ❌ LBS定位失败");
    }
    Serial.println();
    
    // 5. 显示原始响应数据
    Serial.println("5. 原始响应数据:");
    String rawData = modem->getLBSRawData();
    if (rawData.length() > 0) {
        Serial.println("   " + rawData);
    } else {
        Serial.println("   ❌ 无原始数据");
    }
    Serial.println();
    
    // 6. 网络信息
    Serial.println("6. 网络信息:");
    Serial.printf("   信号强度: %d\n", modem->getCSQ());
    Serial.printf("   运营商: %s\n", modem->getCarrierName().c_str());
    Serial.printf("   网络类型: %s\n", modem->getNetworkType().c_str());
    Serial.printf("   本地IP: %s\n", modem->getLocalIP().c_str());
    Serial.println();
    
    Serial.println("=== Air780EG LBS 调试完成 ===");
}

void testAir780EGLbsCommands(Air780EGModem* modem) {
    if (!modem) {
        Serial.println("❌ Air780EG Modem 指针为空");
        return;
    }
    
    Serial.println("=== Air780EG LBS AT命令测试 ===");
    Serial.println();
    
    // 测试基础AT命令
    Serial.println("1. 测试基础AT命令:");
    Serial.println("   发送: AT");
    String response = modem->sendATWithResponse("AT", 1000);
    Serial.println("   响应: " + response);
    Serial.println();
    
    // 测试网络状态
    Serial.println("2. 测试网络状态:");
    Serial.println("   发送: AT+CREG?");
    response = modem->sendATWithResponse("AT+CREG?", 3000);
    Serial.println("   响应: " + response);
    Serial.println();
    
    // 测试信号强度
    Serial.println("3. 测试信号强度:");
    Serial.println("   发送: AT+CSQ");
    response = modem->sendATWithResponse("AT+CSQ", 3000);
    Serial.println("   响应: " + response);
    Serial.println();
    
    // 测试LBS服务器配置
    Serial.println("4. 测试LBS服务器配置:");
    Serial.println("   发送: AT+GSMLOCFG?");
    response = modem->sendATWithResponse("AT+GSMLOCFG?", 3000);
    Serial.println("   响应: " + response);
    Serial.println();
    
    // 设置LBS服务器
    Serial.println("5. 设置LBS服务器:");
    Serial.println("   发送: AT+GSMLOCFG=\"bs.openluat.com\",12411");
    response = modem->sendATWithResponse("AT+GSMLOCFG=\"bs.openluat.com\",12411", 5000);
    Serial.println("   响应: " + response);
    Serial.println();
    
    // 测试PDP上下文状态
    Serial.println("6. 测试PDP上下文状态:");
    Serial.println("   发送: AT+SAPBR=2,1");
    response = modem->sendATWithResponse("AT+SAPBR=2,1", 3000);
    Serial.println("   响应: " + response);
    Serial.println();
    
    Serial.println("=== AT命令测试完成 ===");
}

#endif // USE_AIR780EG_GSM
