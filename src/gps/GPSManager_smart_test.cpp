/*
 * GPSManager 智能定位切换测试工具
 * 测试 GNSS 失败时自动切换到 LBS 的功能
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
            return "GNSS+LBS智能";
        default:
            return "未知";
    }
}

void testSmartLocationSwitching() {
    Serial.println("=== GPSManager 智能定位切换测试 ===");
    Serial.println();
    
    GPSManager& manager = GPSManager::getInstance();
    
    // 1. 显示当前状态
    Serial.println("1. 当前状态:");
    Serial.printf("   GPSManager初始化: %s\n", manager.isReady() ? "✅ 是" : "❌ 否");
    Serial.printf("   定位模式: %s\n", getLocationModeString(manager.getLocationMode()).c_str());
    Serial.printf("   GNSS模块类型: %s\n", manager.getGNSSModuleTypeString().c_str());
    Serial.println();
    
    // 2. 初始化GPSManager（如果需要）
    if (!manager.isReady()) {
        Serial.println("2. 初始化GPSManager:");
        manager.init();
        Serial.printf("   初始化结果: %s\n", manager.isReady() ? "✅ 成功" : "❌ 失败");
        Serial.println();
    }
    
    // 3. 设置为智能GNSS+LBS模式
    Serial.println("3. 设置智能定位模式:");
    Serial.println("   设置为GNSS+LBS智能混合模式...");
    manager.setLocationMode(LocationMode::GNSS_WITH_LBS);
    Serial.printf("   当前模式: %s\n", getLocationModeString(manager.getLocationMode()).c_str());
    Serial.printf("   GNSS启用: %s\n", manager.isGNSSEnabled() ? "✅ 是" : "❌ 否");
    Serial.printf("   LBS启用: %s\n", manager.isLBSEnabled() ? "✅ 是" : "❌ 否");
    Serial.println();
    
    // 4. 显示智能定位状态
    Serial.println("4. 智能定位状态:");
    Serial.printf("   定位状态: %s\n", manager.getLocationStatusString().c_str());
    Serial.printf("   使用LBS备用: %s\n", manager.isUsingLBSFallback() ? "✅ 是" : "❌ 否");
    Serial.printf("   GNSS定位丢失: %s\n", manager.isGNSSFixLost() ? "✅ 是" : "❌ 否");
    Serial.printf("   距离上次GNSS定位: %lu 秒\n", manager.getTimeSinceLastGNSSFix() / 1000);
    Serial.printf("   GNSS失败持续时间: %lu 秒\n", manager.getGNSSFailureDuration() / 1000);
    Serial.println();
    
    // 5. 显示当前定位数据
    Serial.println("5. 当前定位数据:");
    
    // GNSS数据
    gps_data_t gnssData = manager.getGPSData();
    Serial.println("   GNSS数据:");
    if (manager.isGPSDataValid()) {
        Serial.printf("     纬度: %.6f\n", gnssData.latitude);
        Serial.printf("     经度: %.6f\n", gnssData.longitude);
        Serial.printf("     高度: %.1f 米\n", gnssData.altitude);
        Serial.printf("     卫星数: %d\n", gnssData.satellites);
        Serial.printf("     HDOP: %.2f\n", gnssData.hdop);
        Serial.printf("     定位状态: %s\n", manager.isGNSSFixed() ? "✅ 已定位" : "❌ 未定位");
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
    
    // 6. 运行智能切换监控循环
    Serial.println("6. 智能切换监控测试:");
    Serial.println("   运行30次监控循环，观察智能切换行为...");
    Serial.println("   (每次间隔3秒，总计90秒)");
    Serial.println();
    
    for (int i = 0; i < 30; i++) {
        Serial.printf("   [%02d/30] ", i + 1);
        
        unsigned long startTime = millis();
        manager.loop();
        unsigned long endTime = millis();
        
        // 显示当前状态
        bool gnssFixed = manager.isGNSSFixed();
        bool lbsValid = manager.isLBSDataValid();
        bool usingLBSFallback = manager.isUsingLBSFallback();
        
        Serial.printf("GNSS:%s LBS:%s 模式:%s 耗时:%lums",
                     gnssFixed ? "✅" : "❌",
                     lbsValid ? "✅" : "❌",
                     usingLBSFallback ? "备用" : "辅助",
                     endTime - startTime);
        
        // 显示定位数据变化
        gps_data_t currentGnss = manager.getGPSData();
        lbs_data_t currentLbs = manager.getLBSData();
        
        if (gnssFixed) {
            Serial.printf(" GNSS:(%.4f,%.4f)", currentGnss.latitude, currentGnss.longitude);
        }
        if (lbsValid) {
            Serial.printf(" LBS:(%.4f,%.4f)", currentLbs.latitude, currentLbs.longitude);
        }
        
        // 显示智能切换状态变化
        if (manager.isGNSSFixLost()) {
            Serial.printf(" [GNSS失败:%lus]", manager.getGNSSFailureDuration() / 1000);
        }
        
        Serial.println();
        
        delay(3000); // 等待3秒
    }
    Serial.println();
    
    // 7. 显示最终状态
    Serial.println("7. 最终智能定位状态:");
    Serial.printf("   定位状态: %s\n", manager.getLocationStatusString().c_str());
    Serial.printf("   使用LBS备用: %s\n", manager.isUsingLBSFallback() ? "✅ 是" : "❌ 否");
    Serial.printf("   GNSS定位丢失: %s\n", manager.isGNSSFixLost() ? "✅ 是" : "❌ 否");
    Serial.printf("   距离上次GNSS定位: %lu 秒\n", manager.getTimeSinceLastGNSSFix() / 1000);
    Serial.printf("   GNSS失败持续时间: %lu 秒\n", manager.getGNSSFailureDuration() / 1000);
    Serial.println();
    
    // 8. 数据精度分析
    if (manager.isGPSDataValid() && manager.isLBSDataValid()) {
        Serial.println("8. GNSS与LBS数据精度分析:");
        gps_data_t finalGnss = manager.getGPSData();
        lbs_data_t finalLbs = manager.getLBSData();
        
        float latDiff = abs(finalGnss.latitude - finalLbs.latitude);
        float lonDiff = abs(finalGnss.longitude - finalLbs.longitude);
        
        Serial.printf("   纬度差异: %.6f 度\n", latDiff);
        Serial.printf("   经度差异: %.6f 度\n", lonDiff);
        
        // 估算距离差异（简单计算）
        float distanceKm = sqrt(latDiff * latDiff + lonDiff * lonDiff) * 111.0;
        Serial.printf("   估算距离差异: %.2f 公里\n", distanceKm);
        
        if (distanceKm < 1.0) {
            Serial.println("   ✅ GNSS与LBS数据一致性良好");
        } else if (distanceKm < 10.0) {
            Serial.println("   ⚠️  GNSS与LBS数据存在一定差异");
        } else {
            Serial.println("   ❌ GNSS与LBS数据差异较大");
        }
    } else {
        Serial.println("8. 无法进行精度分析（缺少有效数据）");
    }
    Serial.println();
    
    // 9. 智能切换效果评估
    Serial.println("9. 智能切换效果评估:");
    if (manager.isUsingLBSFallback()) {
        Serial.println("   ✅ 智能切换功能正常工作");
        Serial.println("   📍 当前使用LBS备用定位，说明GNSS信号较弱");
        Serial.println("   🔄 系统会持续监控GNSS状态，信号恢复后自动切换回GNSS");
    } else if (manager.isGNSSFixed()) {
        Serial.println("   ✅ GNSS定位正常工作");
        Serial.println("   📍 当前使用GNSS主定位，LBS作为辅助");
        Serial.println("   🛡️ 智能切换功能待命，GNSS失败时会自动启用LBS备用");
    } else {
        Serial.println("   ⚠️  当前所有定位源都无效");
        Serial.println("   🔧 建议检查硬件连接和网络状态");
    }
    Serial.println();
    
    Serial.println("=== GPSManager 智能定位切换测试完成 ===");
}
