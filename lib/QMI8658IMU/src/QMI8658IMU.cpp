#include "QMI8658IMU.h"
#include <ArduinoJson.h>
#include <string.h>

// 静态成员变量
volatile bool QMI8658IMU::motionInterruptFlag = false;

void IRAM_ATTR QMI8658IMU::motionISR()
{
    QMI8658IMU::motionInterruptFlag = true;
}

QMI8658IMU::QMI8658IMU(int motionIntPin, int sdaPin, int sclPin, uint8_t i2cAddress)
    : motionIntPin(motionIntPin),
      _sdaPin(sdaPin),
      _sclPin(sclPin),
      _i2cAddress(i2cAddress),
      _debug(false),
      _lastDebugPrintTime(0),
      _highPrecisionMode(false),
      _dataCallback(nullptr),
      _statusCallback(nullptr)
{
    this->motionIntPin = motionIntPin;
    motionThreshold = MOTION_DETECTION_THRESHOLD_DEFAULT;
    motionDetectionEnabled = false;
    sampleWindow = MOTION_DETECTION_WINDOW_DEFAULT;
    
    // 初始化数据结构
    memset(&_imu_data, 0, sizeof(imu_data_t));
}

QMI8658IMU::~QMI8658IMU()
{
    if (motionIntPin >= 0) {
        detachInterrupt(motionIntPin);
    }
}

void QMI8658IMU::debugPrint(const String &message)
{
    if (_debug)
    {
        Serial.println("[QMI8658IMU] [debug] " + message);
    }
}

bool QMI8658IMU::initI2C(TwoWire& wire)
{
    if (_sdaPin >= 0 && _sclPin >= 0) {
        if (!wire.begin(_sdaPin, _sclPin)) {
            Serial.println("[QMI8658IMU] ❌ I2C初始化失败");
            return false;
        }
        wire.setClock(50000); // 50kHz
        Serial.printf("[QMI8658IMU] I2C已初始化 - SDA:%d, SCL:%d\n", _sdaPin, _sclPin);
    }
    return true;
}

bool QMI8658IMU::begin(TwoWire& wire)
{
    Serial.println("[QMI8658IMU] 初始化开始");
    
    // 初始化I2C（如果提供了引脚）
    if (_sdaPin >= 0 && _sclPin >= 0) {
        if (!initI2C(wire)) {
            return false;
        }
    }
    
    Serial.printf("[QMI8658IMU] 使用I2C总线, INT引脚: %d, 地址: 0x%02X\n", motionIntPin, _i2cAddress);
    
    // 初始化传感器
    if (!qmi.begin(wire, _i2cAddress, _sdaPin, _sclPin))
    {
        Serial.println("[QMI8658IMU] 初始化失败");
        for (int i = 0; i < 3; i++)
        {
            delay(1000);
            Serial.println("[QMI8658IMU] 重试初始化...");
            if (qmi.begin(wire, _i2cAddress, _sdaPin, _sclPin))
            {
                Serial.println("[QMI8658IMU] 重试成功");
                break;
            }
            if (i == 2)
            {
                Serial.println("[QMI8658IMU] 多次重试失败");
                return false;
            }
        }
    }

    Serial.print("[QMI8658IMU] 设备ID: 0x");
    Serial.println(qmi.getChipID(), HEX);

    // 配置陀螺仪 - 896.8Hz采样率（先配置陀螺仪）
    qmi.configGyroscope(
        SensorQMI8658::GYR_RANGE_1024DPS,  // ±1024°/s量程
        SensorQMI8658::GYR_ODR_896_8Hz,    // 896.8Hz采样率
        SensorQMI8658::LPF_MODE_3          // 低通滤波器模式3
    );
    qmi.enableGyroscope();
    Serial.println("[QMI8658IMU] 陀螺仪已启用 - 896.8Hz");

    // 配置加速度计 - 1000Hz采样率（与陀螺仪接近）
    qmi.configAccelerometer(SensorQMI8658::ACC_RANGE_4G, SensorQMI8658::ACC_ODR_1000Hz);
    qmi.enableAccelerometer();
    Serial.println("[QMI8658IMU] 加速度计已启用 - 1000Hz");
    
    // 根据官方文档：同时启用加速度计和陀螺仪时，输出频率基于陀螺仪频率
    Serial.println("[QMI8658IMU] 数据输出频率将基于陀螺仪频率 (896.8Hz)");

    // 配置三轴任意运动检测
    uint8_t modeCtrl = SensorQMI8658::ANY_MOTION_EN_X |
                       SensorQMI8658::ANY_MOTION_EN_Y |
                       SensorQMI8658::ANY_MOTION_EN_Z;
    float AnyMotionXThr = 1.0 * 1000; // 1.0g, 单位mg
    float AnyMotionYThr = 1.0 * 1000;
    float AnyMotionZThr = 1.0 * 1000;
    uint8_t AnyMotionWindow = 16; // 连续16个采样点
    qmi.configMotion(modeCtrl,
                     AnyMotionXThr, AnyMotionYThr, AnyMotionZThr, AnyMotionWindow,
                     0, 0, 0, 0, 0, 0);
    qmi.enableMotionDetect(SensorQMI8658::INTERRUPT_PIN_1);
    
    // 配置中断引脚
    if (motionIntPin >= 0)
    {
        pinMode(motionIntPin, INPUT_PULLUP);
        attachInterrupt(motionIntPin, QMI8658IMU::motionISR, CHANGE);
        Serial.printf("[QMI8658IMU] 运动检测中断已绑定: GPIO%d\n", motionIntPin);
    }
    Serial.println("[QMI8658IMU] 运动检测初始化完成");
    Serial.println("[QMI8658IMU] ✅ 初始化完成");
    
    if (_statusCallback) {
        _statusCallback(true);
    }
    
    return true;
}

void QMI8658IMU::configureMotionDetection(float threshold)
{
    // 配置三轴任意运动检测
    uint8_t modeCtrl = SensorQMI8658::ANY_MOTION_EN_X |
                       SensorQMI8658::ANY_MOTION_EN_Y |
                       SensorQMI8658::ANY_MOTION_EN_Z;

    // 将g转换为mg
    float thresholdMg = threshold * 1000;

    qmi.configMotion(modeCtrl,
                     thresholdMg, thresholdMg, thresholdMg,
                     MOTION_DETECTION_WINDOW_DEFAULT,
                     0, 0, 0, 0, 0, 0);

    qmi.enableMotionDetect(SensorQMI8658::INTERRUPT_PIN_1);
}

void QMI8658IMU::disableMotionDetection()
{
    if (!motionDetectionEnabled)
        return;

    if (motionIntPin >= 0)
    {
        detachInterrupt(motionIntPin);
    }

    qmi.disableMotionDetect();
    motionDetectionEnabled = false;
    Serial.println("[QMI8658IMU] 运动检测已禁用");
}

bool QMI8658IMU::configureForDeepSleep()
{
    // 禁用当前的运动检测中断
    if (motionIntPin >= 0)
    {
        detachInterrupt(motionIntPin);
    }

    // 使用官方的WakeOnMotion配置，专门用于深度睡眠唤醒
    int result = qmi.configWakeOnMotion(
        255,                                   // 255mg阈值
        SensorQMI8658::ACC_ODR_LOWPOWER_128Hz, // 低功耗模式
        SensorQMI8658::INTERRUPT_PIN_1,        // 使用中断引脚1
        1,                                     // 默认引脚值为1
        0x30                                   // 增加抑制时间，减少误触发
    );

    if (result != DEV_WIRE_NONE)
    {
        Serial.println("[QMI8658IMU] WakeOnMotion配置失败");
        return false;
    }

    // 重新配置中断引脚为CHANGE触发
    if (motionIntPin >= 0)
    {
        pinMode(motionIntPin, INPUT_PULLUP);
        attachInterrupt(motionIntPin, QMI8658IMU::motionISR, CHANGE);
    }

    Serial.println("[QMI8658IMU] 已配置为WakeOnMotion深度睡眠模式 (阈值=255mg, 低功耗128Hz)");
    return true;
}

bool QMI8658IMU::restoreFromDeepSleep()
{
    // 唤醒后适当延时，确保I2C/IMU电源和时钟ready
    delay(500);

    // 重置设备
    if (!qmi.reset())
    {
        Serial.println("[QMI8658IMU] 重置失败");
        return false;
    }
    delay(50);

    // 重新配置正常的加速度计
    qmi.configAccelerometer(SensorQMI8658::ACC_RANGE_4G, SensorQMI8658::ACC_ODR_500Hz);
    qmi.enableAccelerometer();
    delay(50);

    // 重新启用陀螺仪
    setGyroEnabled(true);
    delay(50);

    // 恢复正常的运动检测配置（如果之前启用了的话）
    if (motionDetectionEnabled)
    {
        configureMotionDetection(motionThreshold);
        delay(50);
    }

    Serial.println("[QMI8658IMU] 已从WakeOnMotion模式恢复到正常模式");
    return true;
}

bool QMI8658IMU::checkWakeOnMotionEvent()
{
    uint8_t status = qmi.getStatusRegister();

    if (status & SensorQMI8658::EVENT_WOM_MOTION)
    {
        Serial.println("[QMI8658IMU] 检测到WakeOnMotion事件");
        return true;
    }

    return false;
}

bool QMI8658IMU::isMotionDetected()
{
    if (!motionDetectionEnabled)
        return false;

    if (motionInterruptFlag)
    {
        motionInterruptFlag = false;
        uint8_t status = qmi.getStatusRegister();
        return (status & SensorQMI8658::EVENT_ANY_MOTION) != 0;
    }

    return false;
}

void QMI8658IMU::setAccelPowerMode(uint8_t mode)
{
    switch (mode)
    {
    case 0: // 低功耗
        qmi.configAccelerometer(SensorQMI8658::ACC_RANGE_4G, SensorQMI8658::ACC_ODR_125Hz);
        Serial.println("[QMI8658IMU] 加速度计设置为低功耗模式");
        break;
    case 1: // 正常
        qmi.configAccelerometer(SensorQMI8658::ACC_RANGE_4G, SensorQMI8658::ACC_ODR_500Hz);
        Serial.println("[QMI8658IMU] 加速度计设置为正常模式");
        break;
    case 2: // 高性能
        qmi.configAccelerometer(SensorQMI8658::ACC_RANGE_4G, SensorQMI8658::ACC_ODR_1000Hz);
        Serial.println("[QMI8658IMU] 加速度计设置为高性能模式");
        break;
    }
}

void QMI8658IMU::setGyroEnabled(bool enabled)
{
    if (enabled)
    {
        qmi.configGyroscope(
            SensorQMI8658::GYR_RANGE_1024DPS,
            SensorQMI8658::GYR_ODR_896_8Hz,
            SensorQMI8658::LPF_MODE_3
        );
        qmi.enableGyroscope();
        Serial.println("[QMI8658IMU] 陀螺仪已启用 - 896.8Hz");
    }
    else
    {
        qmi.disableGyroscope();
        Serial.println("[QMI8658IMU] 陀螺仪已禁用");
    }
}

void QMI8658IMU::loop()
{
    // 读取加速度计数据
    float new_accel_x, new_accel_y, new_accel_z;
    bool accel_success = qmi.getAccelerometer(new_accel_x, new_accel_y, new_accel_z);
    
    // 读取陀螺仪数据
    float new_gyro_x, new_gyro_y, new_gyro_z;
    bool gyro_success = qmi.getGyroscope(new_gyro_x, new_gyro_y, new_gyro_z);
    
    // 更新数据
    _imu_data.accel_x = new_accel_x;
    _imu_data.accel_y = new_accel_y;
    _imu_data.accel_z = new_accel_z;
    _imu_data.gyro_x = new_gyro_x;
    _imu_data.gyro_y = new_gyro_y;
    _imu_data.gyro_z = new_gyro_z;
    _imu_data.temperature = qmi.getTemperature_C();

    // 应用传感器旋转（如果定义了）
#ifdef IMU_ROTATION
    float temp = _imu_data.accel_x;
    _imu_data.accel_x = _imu_data.accel_y;
    _imu_data.accel_y = -temp;
#endif

    // 保持简单的互补滤波用于基础姿态估计
    float roll_acc = atan2(_imu_data.accel_y, _imu_data.accel_z) * 180 / M_PI;
    float pitch_acc = atan2(-_imu_data.accel_x, sqrt(_imu_data.accel_y * _imu_data.accel_y + _imu_data.accel_z * _imu_data.accel_z)) * 180 / M_PI;
    
    _imu_data.roll = 0.5f * (_imu_data.roll + _imu_data.gyro_x * IMU_DT) + 0.5f * roll_acc;
    _imu_data.pitch = 0.5f * (_imu_data.pitch + _imu_data.gyro_y * IMU_DT) + 0.5f * pitch_acc;

    // 调用数据回调
    if (_dataCallback) {
        _dataCallback(_imu_data);
    }

    // 处理运动检测中断
    if (motionDetectionEnabled && isMotionDetected())
    {
        static unsigned long lastMotionTime = 0;
        unsigned long now = millis();
        if (now - lastMotionTime > MOTION_DETECTION_DEBOUNCE_MS)
        {
            Serial.printf("[QMI8658IMU] 检测到运动! 中断引脚: GPIO%d\n", motionIntPin);
            lastMotionTime = now;
        }
    }
}

bool QMI8658IMU::detectMotion()
{
    float accelMagnitude = sqrt(
        _imu_data.accel_x * _imu_data.accel_x +
        _imu_data.accel_y * _imu_data.accel_y +
        _imu_data.accel_z * _imu_data.accel_z);
    float delta = fabs(accelMagnitude - lastAccelMagnitude);
    lastAccelMagnitude = accelMagnitude;
    accumulatedDelta += delta;
    sampleIndex++;
    if (sampleIndex >= sampleWindow)
    {
        float averageDelta = accumulatedDelta / sampleWindow;
        bool motionDetected = averageDelta > (motionThreshold * 0.8);
        accumulatedDelta = 0;
        sampleIndex = 0;
        return motionDetected;
    }
    return false;
}

void QMI8658IMU::printImuData()
{
    Serial.printf("imu_data: roll=%.2f, pitch=%.2f, yaw=%.2f, temp=%.1f°C | accel=(%.2f,%.2f,%.2f)g | gyro=(%.1f,%.1f,%.1f)°/s\n",
        _imu_data.roll, _imu_data.pitch, _imu_data.yaw, _imu_data.temperature,
        _imu_data.accel_x, _imu_data.accel_y, _imu_data.accel_z,
        _imu_data.gyro_x, _imu_data.gyro_y, _imu_data.gyro_z);
}

String imu_data_to_json(const imu_data_t& imu_data)
{
    StaticJsonDocument<256> doc;
    doc["ax"] = imu_data.accel_x;
    doc["ay"] = imu_data.accel_y;
    doc["az"] = imu_data.accel_z;
    doc["gx"] = imu_data.gyro_x;
    doc["gy"] = imu_data.gyro_y;
    doc["gz"] = imu_data.gyro_z;
    doc["roll"] = imu_data.roll;
    doc["pitch"] = imu_data.pitch;
    doc["yaw"] = imu_data.yaw;
    doc["temp"] = imu_data.temperature;
    return doc.as<String>();
}

float QMI8658IMU::getAccelMagnitude() const {
    return sqrt(_imu_data.accel_x * _imu_data.accel_x + 
                _imu_data.accel_y * _imu_data.accel_y + 
                _imu_data.accel_z * _imu_data.accel_z);
}

void QMI8658IMU::setHighPrecisionMode(bool enabled) {
    if (_highPrecisionMode == enabled) {
        return;
    }
    
    _highPrecisionMode = enabled;
    
    if (enabled) {
        Serial.println("[QMI8658IMU] 切换到高精度模式");
        
        qmi.configGyroscope(
            SensorQMI8658::GYR_RANGE_512DPS,
            SensorQMI8658::GYR_ODR_896_8Hz,
            SensorQMI8658::LPF_MODE_0
        );
        
        qmi.configAccelerometer(
            SensorQMI8658::ACC_RANGE_2G,
            SensorQMI8658::ACC_ODR_1000Hz
        );
        
        configureMotionDetection(0.02f);
        
    } else {
        Serial.println("[QMI8658IMU] 切换到标准精度模式");
        
        qmi.configGyroscope(
            SensorQMI8658::GYR_RANGE_1024DPS,
            SensorQMI8658::GYR_ODR_896_8Hz,
            SensorQMI8658::LPF_MODE_3
        );
        
        qmi.configAccelerometer(
            SensorQMI8658::ACC_RANGE_4G,
            SensorQMI8658::ACC_ODR_1000Hz
        );
        
        configureMotionDetection(motionThreshold);
    }
    
    Serial.printf("[QMI8658IMU] 高精度模式: %s\n", enabled ? "启用" : "禁用");
}

