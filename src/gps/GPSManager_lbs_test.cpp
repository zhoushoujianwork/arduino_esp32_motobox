/*
 * GPSManager LBS 集成测试工具
 * 测试 GPSManager 中的 LBS 功能集成
 */

#include "config.h"
#include "GPSManager.h"

// 辅助函数：获取定位模式字符串
static String getLocationModeString(LocationMode mode) {
    switch (mode) {
        case LocationMode::NONE:
            return "无定位";
        case LocationMode::GPS_ONLY:
            return "仅GPS";
        case LocationMode::GNSS_ONLY:
            return "仅GNSS";
        case LocationMode::GNSS_WITH_LBS:
            return "GNSS+LBS";
        default:
            return "未知";
    }
}

void testGPSManagerLBS() {
    Serial.println("=== GPSManager LBS 集成测试 ===");
    Serial.println();
    
    GPSManager& manager = GPSManager::getInstance();
    
    // 1. 显示当前状态
    Serial.println("1. 当前状态:");
    Serial.printf("   GPSManager初始化: %s\n", manager.isReady() ? "✅ 是" : "❌ 否");
    Serial.printf("   定位模式: %s\n", getLocationModeString(manager.getLocationMode()).c_str());
    Serial.printf("   GNSS启用: %s\n", manager.isGNSSEnabled() ? "✅ 是" : "❌ 否");
    Serial.printf("   LBS启用: %s\n", manager.isLBSEnabled() ? "✅ 是" : "❌ 否");
    Serial.printf("   GPS启用: %s\n", manager.isGPSEnabled() ? "✅ 是" : "❌ 否");
    Serial.printf("   GNSS模块类型: %s\n", manager.getGNSSModuleTypeString().c_str());
    Serial.println();
    
    // 2. 初始化GPSManager（如果需要）
    if (!manager.isReady()) {
        Serial.println("2. 初始化GPSManager:");
        manager.init();
        Serial.printf("   初始化结果: %s\n", manager.isReady() ? "✅ 成功" : "❌ 失败");
        Serial.println();
    }
    
    // 3. 设置为GNSS+LBS混合模式
    Serial.println("3. 设置定位模式:");
    Serial.println("   设置为GNSS+LBS混合模式...");
    manager.setLocationMode(LocationMode::GNSS_WITH_LBS);
    Serial.printf("   当前模式: %s\n", getLocationModeString(manager.getLocationMode()).c_str());
    Serial.printf("   GNSS启用: %s\n", manager.isGNSSEnabled() ? "✅ 是" : "❌ 否");
    Serial.printf("   LBS启用: %s\n", manager.isLBSEnabled() ? "✅ 是" : "❌ 否");
    Serial.println();
    
    // 4. 显示当前数据
    Serial.println("4. 当前定位数据:");
    
    // GNSS数据
    gps_data_t gnssData = manager.getGPSData();
    Serial.println("   GNSS数据:");
    if (manager.isGPSDataValid()) {
        Serial.printf("     纬度: %.6f\n", gnssData.latitude);
        Serial.printf("     经度: %.6f\n", gnssData.longitude);
        Serial.printf("     高度: %.1f 米\n", gnssData.altitude);
        Serial.printf("     卫星数: %d\n", gnssData.satellites);
        Serial.printf("     HDOP: %.2f\n", gnssData.hdop);
        Serial.printf("     速度: %.2f km/h\n", gnssData.speed);
    } else {
        Serial.println("     ❌ 无有效GNSS数据");
    }
    
    // LBS数据
    lbs_data_t lbsData = manager.getLBSData();
    Serial.println("   LBS数据:");
    if (manager.isLBSDataValid()) {
        Serial.printf("     纬度: %.6f\n", lbsData.latitude);
        Serial.printf("     经度: %.6f\n", lbsData.longitude);
        Serial.printf("     半径: %d 米\n", lbsData.radius);
        Serial.printf("     状态: %d\n", (int)lbsData.state);
        Serial.printf("     时间戳: %lu\n", lbsData.timestamp);
        Serial.printf("     数据年龄: %lu 秒\n", (millis() - lbsData.timestamp) / 1000);
    } else {
        Serial.println("     ❌ 无有效LBS数据");
    }
    Serial.println();
    
    // 5. 运行数据更新循环
    Serial.println("5. 数据更新测试:");
    Serial.println("   运行10次数据更新循环...");
    
    for (int i = 0; i < 10; i++) {
        Serial.printf("   循环 %d/10: ", i + 1);
        
        unsigned long startTime = millis();
        manager.loop();
        unsigned long endTime = millis();
        
        Serial.printf("耗时 %lu ms", endTime - startTime);
        
        // 检查数据更新
        bool gnssUpdated = manager.isGPSDataValid();
        bool lbsUpdated = manager.isLBSDataValid();
        
        if (gnssUpdated || lbsUpdated) {
            Serial.print(" - 数据更新: ");
            if (gnssUpdated) Serial.print("GNSS ");
            if (lbsUpdated) Serial.print("LBS ");
        }
        
        Serial.println();
        delay(2000); // 等待2秒
    }
    Serial.println();
    
    // 6. 显示最终数据
    Serial.println("6. 最终定位数据:");
    
    // 更新后的GNSS数据
    gnssData = manager.getGPSData();
    Serial.println("   GNSS数据:");
    if (manager.isGPSDataValid()) {
        Serial.printf("     纬度: %.6f\n", gnssData.latitude);
        Serial.printf("     经度: %.6f\n", gnssData.longitude);
        Serial.printf("     高度: %.1f 米\n", gnssData.altitude);
        Serial.printf("     卫星数: %d\n", gnssData.satellites);
        Serial.printf("     HDOP: %.2f\n", gnssData.hdop);
        Serial.printf("     速度: %.2f km/h\n", gnssData.speed);
        Serial.printf("     定位状态: %s\n", manager.isGNSSFixed() ? "✅ 已定位" : "❌ 未定位");
    } else {
        Serial.println("     ❌ 无有效GNSS数据");
    }
    
    // 更新后的LBS数据
    lbsData = manager.getLBSData();
    Serial.println("   LBS数据:");
    if (manager.isLBSDataValid()) {
        Serial.printf("     纬度: %.6f\n", lbsData.latitude);
        Serial.printf("     经度: %.6f\n", lbsData.longitude);
        Serial.printf("     半径: %d 米\n", lbsData.radius);
        Serial.printf("     状态: %d\n", (int)lbsData.state);
        Serial.printf("     时间戳: %lu\n", lbsData.timestamp);
        Serial.printf("     数据年龄: %lu 秒\n", (millis() - lbsData.timestamp) / 1000);
    } else {
        Serial.println("     ❌ 无有效LBS数据");
    }
    Serial.println();
    
    // 7. 数据对比分析
    if (manager.isGPSDataValid() && manager.isLBSDataValid()) {
        Serial.println("7. GNSS与LBS数据对比:");
        float latDiff = abs(gnssData.latitude - lbsData.latitude);
        float lonDiff = abs(gnssData.longitude - lbsData.longitude);
        
        Serial.printf("   纬度差异: %.6f 度\n", latDiff);
        Serial.printf("   经度差异: %.6f 度\n", lonDiff);
        
        // 估算距离差异（简单计算）
        float distanceKm = sqrt(latDiff * latDiff + lonDiff * lonDiff) * 111.0; // 大约每度111公里
        Serial.printf("   估算距离差异: %.2f 公里\n", distanceKm);
        
        if (distanceKm < 1.0) {
            Serial.println("   ✅ GNSS与LBS数据一致性良好");
        } else if (distanceKm < 10.0) {
            Serial.println("   ⚠️  GNSS与LBS数据存在一定差异");
        } else {
            Serial.println("   ❌ GNSS与LBS数据差异较大");
        }
    } else {
        Serial.println("7. 无法进行数据对比（缺少有效数据）");
    }
    Serial.println();
    
    Serial.println("=== GPSManager LBS 集成测试完成 ===");
}
