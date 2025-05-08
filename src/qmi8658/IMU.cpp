#include "IMU.h"

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
        
        // 自动运动检测中断处理，增加500ms节流去抖动
        static unsigned long lastMotionTime = 0;
        if (IMU::motionInterruptFlag) {
            IMU::motionInterruptFlag = false;
            uint8_t status = qmi.getStatusRegister();
            if (status & SensorQMI8658::EVENT_ANY_MOTION) {
                unsigned long now = millis();
                if (now - lastMotionTime > 500) { // 500ms节流
                    Serial.printf("[IMU] 检测到运动! 中断引脚: GPIO%d\n", motionIntPin);
                    lastMotionTime = now;
                }
            }
        }
    }
}
