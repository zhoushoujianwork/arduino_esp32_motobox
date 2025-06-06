#include "IMU.h"
#include <math.h>

#define USE_WIRE

volatile bool IMU::motionInterruptFlag = false;
void IRAM_ATTR IMU::motionISR() {
    IMU::motionInterruptFlag = true;
}

IMU::IMU(int sda, int scl, int motionIntPin)
{
    this->sda = sda;
    this->scl = scl;
    this->motionIntPin = motionIntPin;
    this->motionThreshold = MOTION_DETECTION_THRESHOLD_DEFAULT;
    this->motionDetectionEnabled = false;
    this->sampleWindow = MOTION_DETECTION_WINDOW_DEFAULT;
}

void IMU::begin()
{
    Serial.println("[IMU] 初始化开始");
#ifdef USE_WIRE
    Serial.printf("[IMU] SDA: %d, SCL: %d\n", sda, scl);
    if (!qmi.begin(Wire1, QMI8658_L_SLAVE_ADDRESS, sda, scl))
    {
        Serial.println("[IMU] 初始化失败");
        for (int i = 0; i < 3; i++) {
            delay(1000);
            Serial.println("[IMU] 重试初始化...");
            if (qmi.begin(Wire1, QMI8658_L_SLAVE_ADDRESS, sda, scl)) {
                Serial.println("[IMU] 重试成功");
                break;
            }
            if (i == 2) {
                Serial.println("[IMU] 多次重试失败，系统将重启");
                delay(1000);
                esp_restart();
            }
        }
    }
#else
    if (!qmi.begin(IMU_CS))
    {
        Serial.println("[IMU] 初始化失败 - 请检查接线!");
        for (int i = 0; i < 3; i++) {
            delay(1000);
            Serial.println("[IMU] 重试初始化...");
            if (qmi.begin(IMU_CS)) {
                Serial.println("[IMU] 重试成功");
                break;
            }
            if (i == 2) {
                Serial.println("[IMU] 多次重试失败，系统将重启");
                delay(1000);
                esp_restart();
            }
        }
    }
#endif

    Serial.print("[IMU] 设备ID: 0x");
    Serial.println(qmi.getChipID(), HEX);

    // 推荐检测采样率
    qmi.configAccelerometer(SensorQMI8658::ACC_RANGE_4G, SensorQMI8658::ACC_ODR_500Hz);
    qmi.enableAccelerometer();

    // 配置三轴任意运动检测
    uint8_t modeCtrl = SensorQMI8658::ANY_MOTION_EN_X |
                       SensorQMI8658::ANY_MOTION_EN_Y |
                       SensorQMI8658::ANY_MOTION_EN_Z;
    float AnyMotionXThr = 1.0 * 1000; // 1.0g, 单位mg
    float AnyMotionYThr = 1.0 * 1000;
    float AnyMotionZThr = 1.0 * 1000;
    uint8_t AnyMotionWindow = 16;      // 连续16个采样点
    qmi.configMotion(modeCtrl,
                     AnyMotionXThr, AnyMotionYThr, AnyMotionZThr, AnyMotionWindow,
                     0, 0, 0, 0, 0, 0);
    qmi.enableMotionDetect(SensorQMI8658::INTERRUPT_PIN_1);
    // 配置中断引脚
    if (motionIntPin >= 0) {
        pinMode(motionIntPin, INPUT_PULLUP);
        attachInterrupt(motionIntPin, IMU::motionISR, CHANGE);
        Serial.printf("[IMU] 运动检测中断已绑定: GPIO%d\n", motionIntPin);
    }
    Serial.println("[IMU] 运动检测初始化完成");
}

bool IMU::enableMotionDetection(int intPin, float threshold) {
    if (intPin < 0) return false;
    
    motionIntPin = intPin;
    motionThreshold = threshold;
    
    // 配置运动检测参数
    configureMotionDetection(threshold);
    
    // 配置中断引脚
    pinMode(motionIntPin, INPUT_PULLUP);
    attachInterrupt(motionIntPin, IMU::motionISR, CHANGE);
    
    motionDetectionEnabled = true;
    Serial.printf("[IMU] 运动检测已启用: GPIO%d, 阈值=%.3fg\n", motionIntPin, threshold);
    
    return true;
}

void IMU::configureMotionDetection(float threshold) {
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

void IMU::disableMotionDetection() {
    if (!motionDetectionEnabled) return;
    
    if (motionIntPin >= 0) {
        detachInterrupt(motionIntPin);
    }
    
    qmi.disableMotionDetect();
    motionDetectionEnabled = false;
    Serial.println("[IMU] 运动检测已禁用");
}

bool IMU::configureForDeepSleep() {
    // 禁用当前的运动检测中断
    if (motionIntPin >= 0) {
        detachInterrupt(motionIntPin);
    }
    
    // 使用官方的WakeOnMotion配置，专门用于深度睡眠唤醒
    // 参数：255mg阈值（uint8_t最大值），低功耗128Hz，使用中断引脚1，默认引脚值1，抑制时间0x30
    int result = qmi.configWakeOnMotion(
        255,                                   // 255mg阈值，uint8_t最大值，比默认200mg更保守
        SensorQMI8658::ACC_ODR_LOWPOWER_128Hz, // 低功耗模式
        SensorQMI8658::INTERRUPT_PIN_1,        // 使用中断引脚1
        1,                                      // 默认引脚值为1
        0x30                                   // 增加抑制时间，减少误触发
    );
    
    if (result != DEV_WIRE_NONE) {
        Serial.println("[IMU] WakeOnMotion配置失败");
        return false;
    }
    
    // 重新配置中断引脚为CHANGE触发（官方例子的方式）
    if (motionIntPin >= 0) {
        pinMode(motionIntPin, INPUT_PULLUP);
        attachInterrupt(motionIntPin, IMU::motionISR, CHANGE);  // 使用CHANGE而非特定边沿
    }
    
    Serial.println("[IMU] 已配置为WakeOnMotion深度睡眠模式 (阈值=255mg, 低功耗128Hz)");
    return true;
}

bool IMU::restoreFromDeepSleep() {
    // 唤醒后适当延时，确保I2C/IMU电源和时钟ready
    delay(200); // 200~500ms，根据实际硬件情况可调整
    // 从WakeOnMotion模式恢复到正常模式需要重新初始化
    
    // 重置设备
    if (!qmi.reset()) {
        Serial.println("[IMU] 重置失败");
        return false;
    }
    
    // 重新配置正常的加速度计
    qmi.configAccelerometer(SensorQMI8658::ACC_RANGE_4G, SensorQMI8658::ACC_ODR_500Hz);
    qmi.enableAccelerometer();
    
    // 重新启用陀螺仪
    setGyroEnabled(true);
    
    // 恢复正常的运动检测配置（如果之前启用了的话）
    if (motionDetectionEnabled) {
        configureMotionDetection(motionThreshold);
    }
    
    Serial.println("[IMU] 已从WakeOnMotion模式恢复到正常模式");
    return true;
}

bool IMU::checkWakeOnMotionEvent() {
    // 使用官方例子的方式检查状态
    uint8_t status = qmi.getStatusRegister();
    
    if (status & SensorQMI8658::EVENT_WOM_MOTION) {
        Serial.println("[IMU] 检测到WakeOnMotion事件");
        return true;
    }
    
    return false;
}

bool IMU::isMotionDetected() {
    if (!motionDetectionEnabled) return false;
    
    if (motionInterruptFlag) {
        motionInterruptFlag = false;
        uint8_t status = qmi.getStatusRegister();
        return (status & SensorQMI8658::EVENT_ANY_MOTION) != 0;
    }
    
    return false;
}

void IMU::setAccelPowerMode(uint8_t mode) {
    // 配置加速度计功耗模式
    switch (mode) {
        case 0: // 低功耗
            qmi.configAccelerometer(SensorQMI8658::ACC_RANGE_4G, SensorQMI8658::ACC_ODR_125Hz);
            Serial.println("[IMU] 加速度计设置为低功耗模式");
            break;
        case 1: // 正常
            qmi.configAccelerometer(SensorQMI8658::ACC_RANGE_4G, SensorQMI8658::ACC_ODR_500Hz);
            Serial.println("[IMU] 加速度计设置为正常模式");
            break;
        case 2: // 高性能
            qmi.configAccelerometer(SensorQMI8658::ACC_RANGE_4G, SensorQMI8658::ACC_ODR_1000Hz);
            Serial.println("[IMU] 加速度计设置为高性能模式");
            break;
    }
}

void IMU::setGyroEnabled(bool enabled) {
    if (enabled) {
        qmi.configGyroscope(
            (SensorQMI8658::GyroRange)6,      // GYR_RANGE_1024DPS = 6
            (SensorQMI8658::GyroODR)3,        // GYR_ODR_896_8Hz = 3
            (SensorQMI8658::LpfMode)3         // LPF_MODE_3 = 3
        );
        qmi.enableGyroscope();
        Serial.println("[IMU] 陀螺仪已启用");
    } else {
        qmi.disableGyroscope();
        Serial.println("[IMU] 陀螺仪已禁用");
    }
}

void IMU::loop()
{
    if (qmi.getDataReady())
    {
        device.get_device_state()->imuReady = true;
        qmi.getAccelerometer(device.get_imu_data()->accel_x, device.get_imu_data()->accel_y, device.get_imu_data()->accel_z);

        // 应用传感器旋转（如果定义了）
#if defined(IMU_ROTATION) 
        // 顺时针旋转90度（适用于传感器侧装）
        float temp = device.get_imu_data()->accel_x;
        device.get_imu_data()->accel_x = device.get_imu_data()->accel_y;
        device.get_imu_data()->accel_y = -temp;
#endif

        qmi.getGyroscope(device.get_imu_data()->gyro_x, device.get_imu_data()->gyro_y, device.get_imu_data()->gyro_z);

        // 使用加速度计计算姿态角
        float roll_acc = atan2(device.get_imu_data()->accel_y, device.get_imu_data()->accel_z) * 180 / M_PI;
        float pitch_acc = atan2(-device.get_imu_data()->accel_x, sqrt(device.get_imu_data()->accel_y * device.get_imu_data()->accel_y + device.get_imu_data()->accel_z * device.get_imu_data()->accel_z)) * 180 / M_PI;

        // 使用陀螺仪数据和互补滤波更新姿态角
        device.get_imu_data()->roll = ALPHA * (device.get_imu_data()->roll + device.get_imu_data()->gyro_x * dt) + (1.0 - ALPHA) * roll_acc;
        device.get_imu_data()->pitch = ALPHA * (device.get_imu_data()->pitch + device.get_imu_data()->gyro_y * dt) + (1.0 - ALPHA) * pitch_acc;

        device.get_imu_data()->temperature = qmi.getTemperature_C();
        
        // 处理运动检测中断
        if (motionDetectionEnabled && isMotionDetected()) {
            static unsigned long lastMotionTime = 0;
            unsigned long now = millis();
            if (now - lastMotionTime > MOTION_DETECTION_DEBOUNCE_MS) {
                Serial.printf("[IMU] 检测到运动! 中断引脚: GPIO%d\n", motionIntPin);
                lastMotionTime = now;
            }
        }
    }
}

/**
 * @brief 检测是否有运动
 * @return true: 检测到运动, false: 未检测到
 */
bool IMU::detectMotion()
{
    imu_data_t *imuData = device.get_imu_data();
    float accelMagnitude = sqrt(
        imuData->accel_x * imuData->accel_x +
        imuData->accel_y * imuData->accel_y +
        imuData->accel_z * imuData->accel_z);
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
