#ifndef QMI8658IMU_H
#define QMI8658IMU_H

#include <Wire.h>
#include <SPI.h>
#include <math.h>
#include "SensorQMI8658.hpp"

#define ALPHA 0.98 // 互补滤波的系数，范围在0到1之间
#define IMU_DT 0.01    // 时间间隔，单位是秒（假设采样率为100Hz）

// 运动检测相关参数
#define MOTION_DETECTION_THRESHOLD_DEFAULT 0.0035   // 0.05 适合震动检测，, 静止的量级0.001~0.003
#define MOTION_DETECTION_WINDOW_DEFAULT 32       // 增加窗口大小到32
#define MOTION_DETECTION_DEBOUNCE_MS 200        // 增加去抖时间到200ms

// QMI8658默认I2C地址（如果未定义，使用默认值）
#ifndef QMI8658_L_SLAVE_ADDRESS
#define QMI8658_L_SLAVE_ADDRESS 0x6B
#endif

typedef struct
{
    // 加速度计数据，单位：g
    float accel_x; // X轴加速度
    float accel_y; // Y轴加速度
    float accel_z; // Z轴加速度

    // 陀螺仪数据，单位：°/s
    float gyro_x; // X轴角速度
    float gyro_y; // Y轴角速度
    float gyro_z; // Z轴角速度

    // 姿态角，单位：度
    float roll;  // 横滚角
    float pitch; // 俯仰角
    float yaw;   // 偏航角

    float temperature; // 温度，单位：摄氏度
} imu_data_t;

// 前向声明
class QMI8658IMU;

// 回调函数类型定义（使用函数指针，兼容C风格）
typedef void (*IMUDataCallback)(imu_data_t& data);
typedef void (*IMUStatusCallback)(bool ready);

class QMI8658IMU
{
public:
    QMI8658IMU(int motionIntPin = -1, 
               int sdaPin = -1, 
               int sclPin = -1, 
               uint8_t i2cAddress = QMI8658_L_SLAVE_ADDRESS);
    ~QMI8658IMU();
    
    // 初始化方法
    bool begin(TwoWire& wire = Wire);
    void loop();
    
    // 运动检测中断标志和ISR
    static volatile bool motionInterruptFlag;
    static void IRAM_ATTR motionISR();
    
    // 低功耗相关方法
    void disableMotionDetection();
    bool configureForDeepSleep();
    bool restoreFromDeepSleep();
    bool isMotionDetected();
    bool checkWakeOnMotionEvent();
    
    // 电源管理方法
    void setAccelPowerMode(uint8_t mode);  // 0=低功耗，1=正常，2=高性能
    void setGyroEnabled(bool enabled);
    
    // 运动检测
    bool detectMotion();
    
    // 数据打印
    void printImuData();
    
    // 数据获取方法
    float getAccelX() const { return _imu_data.accel_x; }
    float getAccelY() const { return _imu_data.accel_y; }
    float getAccelZ() const { return _imu_data.accel_z; }
    float getGyroX() const { return _imu_data.gyro_x; }
    float getGyroY() const { return _imu_data.gyro_y; }
    float getGyroZ() const { return _imu_data.gyro_z; }
    float getRoll() const { return _imu_data.roll; }
    float getPitch() const { return _imu_data.pitch; }
    float getYaw() const { return _imu_data.yaw; }
    float getTemperature() const { return _imu_data.temperature; }
    float getAccelMagnitude() const;
    
    // 获取完整数据
    const imu_data_t& getData() const { return _imu_data; }
    
    // 高精度模式
    void setHighPrecisionMode(bool enabled);
    bool isHighPrecisionMode() const { return _highPrecisionMode; }
    
    // 调试
    void setDebug(bool debug) { _debug = debug; }
    
    // 回调函数设置
    void setDataCallback(IMUDataCallback callback) { _dataCallback = callback; }
    void setStatusCallback(IMUStatusCallback callback) { _statusCallback = callback; }

private:
    bool _debug;
    int motionIntPin;           // 运动检测中断引脚
    int _sdaPin;                // I2C SDA引脚
    int _sclPin;                // I2C SCL引脚
    uint8_t _i2cAddress;       // I2C地址
    float motionThreshold;      // 运动检测阈值
    bool motionDetectionEnabled;// 运动检测是否启用
    bool _highPrecisionMode;    // 高精度模式标志
    SensorQMI8658 qmi;          // QMI8658传感器对象
    imu_data_t _imu_data;       // IMU数据
    
    // 回调函数
    IMUDataCallback _dataCallback;
    IMUStatusCallback _statusCallback;
    
    // 配置运动检测参数
    void configureMotionDetection(float threshold);
    
    // 运动检测相关变量
    float lastAccelMagnitude = 0;
    float accumulatedDelta = 0;
    int sampleIndex = 0;
    int sampleWindow = MOTION_DETECTION_WINDOW_DEFAULT;
    
    void debugPrint(const String& message);
    unsigned long _lastDebugPrintTime;
    
    // I2C初始化辅助函数
    bool initI2C(TwoWire& wire);
};

// JSON序列化函数
String imu_data_to_json(const imu_data_t& imu_data);

#endif

