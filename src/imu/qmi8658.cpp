#include "qmi8658.h"

#define USE_WIRE

#ifdef ENABLE_IMU
IMU imu(IMU_SDA_PIN, IMU_SCL_PIN, IMU_INT_PIN);
#endif

imu_data_t imu_data;

volatile bool IMU::motionInterruptFlag = false;
void IRAM_ATTR IMU::motionISR()
{
    IMU::motionInterruptFlag = true;
}

IMU::IMU(int sda, int scl, int motionIntPin)
    : _wire(Wire1)
{
    _debug = true;
    sda = sda;
    scl = scl;
    motionIntPin = motionIntPin;
    motionThreshold = MOTION_DETECTION_THRESHOLD_DEFAULT;
    motionDetectionEnabled = false;
    sampleWindow = MOTION_DETECTION_WINDOW_DEFAULT;
}

void IMU::debugPrint(const String &message)
{
    if (_debug)
    {
        Serial.println(message);
    }
}

void IMU::begin()
{
    Serial.println("[IMU] 初始化完成");
#ifdef USE_WIRE
    Serial.printf("[IMU] SDA: %d, SCL: %d\n", sda, scl);
    if (!qmi.begin(_wire, QMI8658_L_SLAVE_ADDRESS, sda, scl))
    {
        Serial.println("[IMU] 初始化失败");
        for (int i = 0; i < 3; i++)
        {
            delay(1000);
            Serial.println("[IMU] 重试初始化...");
            if (qmi.begin(_wire, QMI8658_L_SLAVE_ADDRESS, sda, scl))
            {
                Serial.println("[IMU] 重试成功");
                break;
            }
            if (i == 2)
            {
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
        for (int i = 0; i < 3; i++)
        {
            delay(1000);
            Serial.println("[IMU] 重试初始化...");
            if (qmi.begin(IMU_CS))
            {
                Serial.println("[IMU] 重试成功");
                break;
            }
            if (i == 2)
            {
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
    uint8_t AnyMotionWindow = 16; // 连续16个采样点
    qmi.configMotion(modeCtrl,
                     AnyMotionXThr, AnyMotionYThr, AnyMotionZThr, AnyMotionWindow,
                     0, 0, 0, 0, 0, 0);
    qmi.enableMotionDetect(SensorQMI8658::INTERRUPT_PIN_1);
    // 配置中断引脚
    if (motionIntPin >= 0)
    {
        pinMode(motionIntPin, INPUT_PULLUP);
        attachInterrupt(motionIntPin, IMU::motionISR, CHANGE);
        Serial.printf("[IMU] 运动检测中断已绑定: GPIO%d\n", motionIntPin);
    }
    Serial.println("[IMU] 运动检测初始化完成");
}

void IMU::configureMotionDetection(float threshold)
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

void IMU::disableMotionDetection()
{
    if (!motionDetectionEnabled)
        return;

    if (motionIntPin >= 0)
    {
        detachInterrupt(motionIntPin);
    }

    qmi.disableMotionDetect();
    motionDetectionEnabled = false;
    Serial.println("[IMU] 运动检测已禁用");
}

bool IMU::configureForDeepSleep()
{
    // 禁用当前的运动检测中断
    if (motionIntPin >= 0)
    {
        detachInterrupt(motionIntPin);
    }

    // 使用官方的WakeOnMotion配置，专门用于深度睡眠唤醒
    // 参数：255mg阈值（uint8_t最大值），低功耗128Hz，使用中断引脚1，默认引脚值1，抑制时间0x30
    int result = qmi.configWakeOnMotion(
        255,                                   // 255mg阈值，uint8_t最大值，比默认200mg更保守
        SensorQMI8658::ACC_ODR_LOWPOWER_128Hz, // 低功耗模式
        SensorQMI8658::INTERRUPT_PIN_1,        // 使用中断引脚1
        1,                                     // 默认引脚值为1
        0x30                                   // 增加抑制时间，减少误触发
    );

    if (result != DEV_WIRE_NONE)
    {
        Serial.println("[IMU] WakeOnMotion配置失败");
        return false;
    }

    // 重新配置中断引脚为CHANGE触发（官方例子的方式）
    if (motionIntPin >= 0)
    {
        pinMode(motionIntPin, INPUT_PULLUP);
        attachInterrupt(motionIntPin, IMU::motionISR, CHANGE); // 使用CHANGE而非特定边沿
    }

    Serial.println("[IMU] 已配置为WakeOnMotion深度睡眠模式 (阈值=255mg, 低功耗128Hz)");
    return true;
}

bool IMU::restoreFromDeepSleep()
{
    // 唤醒后适当延时，确保I2C/IMU电源和时钟ready
    delay(200); // 200~500ms，根据实际硬件情况可调整
    // 从WakeOnMotion模式恢复到正常模式需要重新初始化

    // 重置设备
    if (!qmi.reset())
    {
        Serial.println("[IMU] 重置失败");
        return false;
    }

    // 重新配置正常的加速度计
    qmi.configAccelerometer(SensorQMI8658::ACC_RANGE_4G, SensorQMI8658::ACC_ODR_500Hz);
    qmi.enableAccelerometer();

    // 重新启用陀螺仪
    setGyroEnabled(true);

    // 恢复正常的运动检测配置（如果之前启用了的话）
    if (motionDetectionEnabled)
    {
        configureMotionDetection(motionThreshold);
    }

    Serial.println("[IMU] 已从WakeOnMotion模式恢复到正常模式");
    return true;
}

bool IMU::checkWakeOnMotionEvent()
{
    // 使用官方例子的方式检查状态
    uint8_t status = qmi.getStatusRegister();

    if (status & SensorQMI8658::EVENT_WOM_MOTION)
    {
        Serial.println("[IMU] 检测到WakeOnMotion事件");
        return true;
    }

    return false;
}

bool IMU::isMotionDetected()
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

void IMU::setAccelPowerMode(uint8_t mode)
{
    // 配置加速度计功耗模式
    switch (mode)
    {
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

void IMU::setGyroEnabled(bool enabled)
{
    if (enabled)
    {
        qmi.configGyroscope(
            (SensorQMI8658::GyroRange)6, // GYR_RANGE_1024DPS = 6
            (SensorQMI8658::GyroODR)3,   // GYR_ODR_896_8Hz = 3
            (SensorQMI8658::LpfMode)3    // LPF_MODE_3 = 3
        );
        qmi.enableGyroscope();
        Serial.println("[IMU] 陀螺仪已启用");
    }
    else
    {
        qmi.disableGyroscope();
        Serial.println("[IMU] 陀螺仪已禁用");
    }
}

void IMU::loop()
{
    if (qmi.getDataReady())
    {
        get_device_state()->imuReady = true;
        qmi.getAccelerometer(imu_data.accel_x, imu_data.accel_y, imu_data.accel_z);

        // 应用传感器旋转（如果定义了）
#if defined(IMU_ROTATION)
        // 顺时针旋转90度（适用于传感器侧装）
        float temp = imu_data.accel_x;
        imu_data.accel_x = imu_data.accel_y;
        imu_data.accel_y = -temp;
#endif

        qmi.getGyroscope(imu_data.gyro_x, imu_data.gyro_y, imu_data.gyro_z);

        // 使用加速度计计算姿态角
        float roll_acc = atan2(imu_data.accel_y, imu_data.accel_z) * 180 / M_PI;
        float pitch_acc = atan2(-imu_data.accel_x, sqrt(imu_data.accel_y * imu_data.accel_y + imu_data.accel_z * imu_data.accel_z)) * 180 / M_PI;

        // 使用陀螺仪数据和互补滤波更新姿态角
        imu_data.roll = ALPHA * (imu_data.roll + imu_data.gyro_x * dt) + (1.0 - ALPHA) * roll_acc;
        imu_data.pitch = ALPHA * (imu_data.pitch + imu_data.gyro_y * dt) + (1.0 - ALPHA) * pitch_acc;

        imu_data.temperature = qmi.getTemperature_C();

        // 处理运动检测中断
        if (motionDetectionEnabled && isMotionDetected())
        {
            static unsigned long lastMotionTime = 0;
            unsigned long now = millis();
            if (now - lastMotionTime > MOTION_DETECTION_DEBOUNCE_MS)
            {
                Serial.printf("[IMU] 检测到运动! 中断引脚: GPIO%d\n", motionIntPin);
                lastMotionTime = now;
            }
        }
    }
    if (millis() - _lastDebugPrintTime > 500)
    {
        _lastDebugPrintTime = millis();
        debugPrint("IMU Heading: " + String(imu_data.roll) + ", " + String(imu_data.pitch) + ", " + String(imu_data.yaw));
    }
}

/**
 * @brief 检测是否有运动
 * @return true: 检测到运动, false: 未检测到
 */
bool IMU::detectMotion()
{
    float accelMagnitude = sqrt(
        imu_data.accel_x * imu_data.accel_x +
        imu_data.accel_y * imu_data.accel_y +
        imu_data.accel_z * imu_data.accel_z);
    float delta = fabs(accelMagnitude - lastAccelMagnitude);
    lastAccelMagnitude = accelMagnitude;
    accumulatedDelta += delta;
    sampleIndex++;
    if (sampleIndex >= sampleWindow)
    {
        float averageDelta = accumulatedDelta / sampleWindow;
        Serial.printf("[IMU] 运动检测阈值: %f, 平均加速度变化: %f\n", motionThreshold, averageDelta);
        bool motionDetected = averageDelta > (motionThreshold * 0.8);
        accumulatedDelta = 0;
        sampleIndex = 0;
        return motionDetected;
    }
    return false;
}

/**
 * @brief 打印IMU数据
 */
void IMU::printImuData()
{
    Serial.println("imu_data: " + String(imu_data.roll) + ", " + String(imu_data.pitch) + ", " + String(imu_data.yaw) + ", " + String(imu_data.temperature));
}

// 生成精简版IMU数据JSON
String imu_data_to_json(imu_data_t &imu_data)
{
    StaticJsonDocument<256> doc;
    doc["ax"] = imu_data.accel_x; // X轴加速度
    doc["ay"] = imu_data.accel_y; // Y轴加速度
    doc["az"] = imu_data.accel_z; // Z轴加速度
    doc["gx"] = imu_data.gyro_x;  // X轴角速度
    doc["gy"] = imu_data.gyro_y;  // Y轴角速度
    doc["gz"] = imu_data.gyro_z;  // Z轴角速度
    // doc["mx"] = imu_data.mag_x;            // X轴磁场
    // doc["my"] = imu_data.mag_y;            // Y轴磁场
    // doc["mz"] = imu_data.mag_z;            // Z轴磁场
    doc["roll"] = imu_data.roll;        // 横滚角
    doc["pitch"] = imu_data.pitch;      // 俯仰角
    doc["yaw"] = imu_data.yaw;          // 航向角
    doc["temp"] = imu_data.temperature; // 温度
    return doc.as<String>();
}