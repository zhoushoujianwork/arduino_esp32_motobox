#ifndef IMU_H
#define IMU_H

#include "QMI8658IMU.h"
#include "device.h"
#include "config.h"
#include "utils/I2CManager.h"

// 使用库中定义的 imu_data_t 类型
// 不再重复定义，避免类型冲突

// 全局数据声明（在qmi8658.cpp中定义）
extern imu_data_t imu_data;

// JSON序列化函数声明（在qmi8658.cpp中实现，调用库中的函数）
String imu_data_to_json(imu_data_t& imu_data);

// IMU包装类，使用QMI8658IMU库
class IMU
{
public:
    IMU(int motionIntPin = -1);
    void begin();
    void loop();
    
    // 运动检测中断标志和ISR（转发到库）
    static volatile bool motionInterruptFlag;
    static void IRAM_ATTR motionISR();
    
    // 低功耗相关方法
    void disableMotionDetection();
    bool configureForDeepSleep();
    bool restoreFromDeepSleep();
    bool isMotionDetected();
    bool checkWakeOnMotionEvent();
    
    // 电源管理方法
    void setAccelPowerMode(uint8_t mode);
    void setGyroEnabled(bool enabled);

    bool detectMotion();
    void printImuData();

    // 数据获取方法（转发到库）
    float getAccelX() const { return _imu.getAccelX(); }
    float getAccelY() const { return _imu.getAccelY(); }
    float getAccelZ() const { return _imu.getAccelZ(); }
    float getGyroX() const { return _imu.getGyroX(); }
    float getGyroY() const { return _imu.getGyroY(); }
    float getGyroZ() const { return _imu.getGyroZ(); }
    float getRoll() const { return _imu.getRoll(); }
    float getPitch() const { return _imu.getPitch(); }
    float getYaw() const { return _imu.getYaw(); }
    float getTemperature() const { return _imu.getTemperature(); }
    float getAccelMagnitude() const { return _imu.getAccelMagnitude(); }
    
    void setHighPrecisionMode(bool enabled) { _imu.setHighPrecisionMode(enabled); }
    bool isHighPrecisionMode() const { return _imu.isHighPrecisionMode(); }
    void setDebug(bool debug) { _imu.setDebug(debug); }

private:
    QMI8658IMU _imu;  // 使用库中的实现
    void configureMotionDetection(float threshold);
};

#ifdef ENABLE_IMU
extern IMU imu;
#endif

#endif

// pitch 90 0   -90
