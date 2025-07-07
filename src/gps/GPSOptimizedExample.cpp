/*
 * GPS优化使用示例
 * 展示如何使用优化后的GPS管理器，避免频繁AT调用
 */

#include "GPSManager.h"
#include "GPSDataCache.h"

// 在setup()中初始化
void setupGPSOptimized() {
    Serial.println("=== GPS优化版本初始化 ===");
    
    // 初始化GPS管理器（会自动初始化数据缓存）
    gpsManager.init();
    gpsManager.setDebug(true);
    
    // 启用GNSS模式
    gpsManager.setLocationMode(LocationMode::GNSS_ONLY);
    gpsManager.setGNSSEnabled(true);
    
    Serial.println("GPS管理器初始化完成");
}

// 在loop()中调用
void loopGPSOptimized() {
    // 关键：只需要调用一次gpsManager.loop()
    // 内部会自动每秒调用一次AT+CGNSINF并缓存数据
    gpsManager.loop();
    
    // 获取GPS数据（从缓存读取，不触发AT调用）
    if (gpsManager.isGPSDataValid()) {
        gps_data_t gpsData = gpsManager.getGPSData();
        
        Serial.printf("GPS位置: %.6f, %.6f\n", 
                     gpsData.latitude, gpsData.longitude);
        Serial.printf("高度: %.1fm, 速度: %.1fkm/h\n", 
                     gpsData.altitude, gpsData.speed);
        Serial.printf("卫星数: %d, HDOP: %.1f\n", 
                     gpsManager.getSatelliteCount(), gpsManager.getHDOP());
    }
    
    // 检查数据新鲜度
    if (gpsDataCache.isDataFresh(3000)) { // 3秒内的数据认为是新鲜的
        Serial.println("GPS数据新鲜");
    } else {
        Serial.println("GPS数据过期");
    }
    
    delay(5000); // 5秒检查一次
}

/*
 * 优化效果说明：
 * 
 * 1. 性能优化：
 *    - 原来：每次获取GPS数据都调用AT+CGNSINF（可能每秒多次）
 *    - 现在：统一每秒调用一次AT+CGNSINF，数据缓存供多次使用
 * 
 * 2. 代码简化：
 *    - 不需要手动管理AT调用频率
 *    - 不需要担心重复调用的性能问题
 *    - 统一的数据获取接口
 * 
 * 3. 数据一致性：
 *    - 同一秒内多次获取的数据保持一致
 *    - 避免了数据不同步的问题
 * 
 * 4. 资源节约：
 *    - 减少串口通信次数
 *    - 降低CPU占用
 *    - 提高系统响应性
 */
