#include "IMU.h"

#define USE_WIRE

IMU::IMU(int sda, int scl)
{
    this->sda = sda;
    this->scl = scl;
    this->motionIntPin = -1; // 默认不使用中断引脚
}

void IMU::begin()
{
    Serial.println("IMU init start");
#ifdef USE_WIRE
    // Using WIRE !!
    if (!qmi.begin(Wire, QMI8658_L_SLAVE_ADDRESS, sda, scl))
    {
        Serial.println("IMU init failed");
        while (1)
        {
            delay(5000);
            Serial.println("IMU init failed, restart");
            esp_restart();
        }
    }
#else
    if (!qmi.begin(IMU_CS))
    {
        Serial.println("Failed to find QMI8658 - check your wiring!");
        while (1)
        {
            Serial.println("IMU init failed, restart");
            delay(1000);
            esp_restart();
        }
    }
#endif

    /* Get chip id*/
    Serial.print("Device ID:");
    Serial.println(qmi.getChipID(), HEX);

    qmi.configAccelerometer(
        /*
         * ACC_RANGE_2G
         * ACC_RANGE_4G
         * ACC_RANGE_8G
         * ACC_RANGE_16G
         * */
        SensorQMI8658::ACC_RANGE_4G,
        /*
         * ACC_ODR_1000H
         * ACC_ODR_500Hz
         * ACC_ODR_250Hz
         * ACC_ODR_125Hz
         * ACC_ODR_62_5Hz
         * ACC_ODR_31_25Hz
         * ACC_ODR_LOWPOWER_128Hz
         * ACC_ODR_LOWPOWER_21Hz
         * ACC_ODR_LOWPOWER_11Hz
         * ACC_ODR_LOWPOWER_3H
         * */
        SensorQMI8658::ACC_ODR_1000Hz,
        /*
         *  LPF_MODE_0     //2.66% of ODR
         *  LPF_MODE_1     //3.63% of ODR
         *  LPF_MODE_2     //5.39% of ODR
         *  LPF_MODE_3     //13.37% of ODR
         * */
        SensorQMI8658::LPF_MODE_1);

    qmi.configGyroscope(
        /*
         * GYR_RANGE_16DPS
         * GYR_RANGE_32DPS
         * GYR_RANGE_64DPS
         * GYR_RANGE_128DPS
         * GYR_RANGE_256DPS
         * GYR_RANGE_512DPS
         * GYR_RANGE_1024DPS
         * */
        SensorQMI8658::GYR_RANGE_1024DPS,
        /*
         * GYR_ODR_7174_4Hz
         * GYR_ODR_3587_2Hz
         * GYR_ODR_1793_6Hz
         * GYR_ODR_896_8Hz
         * GYR_ODR_448_4Hz
         * GYR_ODR_224_2Hz
         * GYR_ODR_112_1Hz
         * GYR_ODR_56_05Hz
         * GYR_ODR_28_025H
         * */
        SensorQMI8658::GYR_ODR_1793_6Hz,
        /*
         *  LPF_MODE_0     //2.66% of ODR
         *  LPF_MODE_1     //3.63% of ODR
         *  LPF_MODE_2     //5.39% of ODR
         *  LPF_MODE_3     //13.37% of ODR
         * */
        SensorQMI8658::LPF_MODE_1);

    /*
     * If both the accelerometer and gyroscope sensors are turned on at the same time,
     * the output frequency will be based on the gyroscope output frequency.
     * The example configuration is 896.8HZ output frequency,
     * so the acceleration output frequency is also limited to 896.8HZ
     * */
    qmi.enableGyroscope();
    qmi.enableAccelerometer();

    // Print register configuration information
    qmi.dumpCtrlRegister();

    Serial.println("IMU init success");
}

void IMU::loop()
{
    if (qmi.getDataReady())
    {
        device.get_device_state()->imuReady = true;
        qmi.getAccelerometer(device.get_imu_data()->accel_x, device.get_imu_data()->accel_y, device.get_imu_data()->accel_z);

        // 选择需要的旋转方式（取消注释其中一种）：

        // 1. 顺时针旋转90度（适用于传感器侧装）
        float temp = device.get_imu_data()->accel_x;
        device.get_imu_data()->accel_x = device.get_imu_data()->accel_y;
        device.get_imu_data()->accel_y = -temp;

        // 2. 逆时针旋转90度
        // float temp = imu_data.accel_x;
        // imu_data.accel_x = -imu_data.accel_y;
        // imu_data.accel_y = temp;

        // 3. 旋转180度
        // imu_data.accel_x = -imu_data.accel_x;
        // imu_data.accel_y = -imu_data.accel_y;

        qmi.getGyroscope(device.get_imu_data()->gyro_x, device.get_imu_data()->gyro_y, device.get_imu_data()->gyro_z);

        // 使用加速度计计算姿态角
        float roll_acc = atan2(device.get_imu_data()->accel_y, device.get_imu_data()->accel_z) * 180 / M_PI;
        float pitch_acc = atan2(-device.get_imu_data()->accel_x, sqrt(device.get_imu_data()->accel_y * device.get_imu_data()->accel_y + device.get_imu_data()->accel_z * device.get_imu_data()->accel_z)) * 180 / M_PI;

        // 使用陀螺仪数据和互补滤波更新姿态角
#define ALPHA 0.7
        device.get_imu_data()->roll = ALPHA * (device.get_imu_data()->roll + device.get_imu_data()->gyro_x * dt) + (1.0 - ALPHA) * roll_acc;
        device.get_imu_data()->pitch = ALPHA * (device.get_imu_data()->pitch + device.get_imu_data()->gyro_y * dt) + (1.0 - ALPHA) * pitch_acc;

        device.get_imu_data()->temperature = qmi.getTemperature_C();
    }
}

bool IMU::enableMotionDetection(int intPin, float threshold)
{
    // 保存中断引脚号
    this->motionIntPin = intPin;
    
    // 设置中断引脚为输入模式
    pinMode(intPin, INPUT);
    
    Serial.println("[IMU] 开始配置运动检测...");

    // 首先确保加速度计已启用
    if (!qmi.isEnableAccelerometer()) {
        qmi.enableAccelerometer();
        Serial.println("[IMU] 已启用加速度计");
    }

    // 配置任意运动检测（ANY_MOTION）
    uint8_t motionCtrl = 0;
    motionCtrl |= SensorQMI8658::ANY_MOTION_EN_X;  // 启用X轴检测
    motionCtrl |= SensorQMI8658::ANY_MOTION_EN_Y;  // 启用Y轴检测
    motionCtrl |= SensorQMI8658::ANY_MOTION_EN_Z;  // 启用Z轴检测
    
    // 配置无运动检测（NO_MOTION）
    motionCtrl |= SensorQMI8658::NO_MOTION_EN_X;   // 启用X轴检测
    motionCtrl |= SensorQMI8658::NO_MOTION_EN_Y;   // 启用Y轴检测
    motionCtrl |= SensorQMI8658::NO_MOTION_EN_Z;   // 启用Z轴检测
    
    // 使用逻辑OR关系，任一轴检测到运动就触发
    // 默认任意运动检测使用逻辑OR，无需额外配置
    
    // 阈值设置 - 将g值转换为更适合的范围
    // 注意：阈值设置取决于传感器量程，这里使用的是ACC_RANGE_4G
    float motionThreshold = threshold;  // g值，例如0.05g

    // 配置任意运动检测参数
    int result = qmi.configMotion(
        motionCtrl,           // 控制寄存器
        motionThreshold,      // X轴阈值
        motionThreshold,      // Y轴阈值
        motionThreshold,      // Z轴阈值
        5,                    // 任意运动检测窗口 (1-255)
        0.0,                  // 无运动X阈值，设为0不启用
        0.0,                  // 无运动Y阈值，设为0不启用
        0.0,                  // 无运动Z阈值，设为0不启用
        0,                    // 无运动窗口
        0,                    // 显著运动等待窗口
        0                     // 显著运动确认窗口
    );
    
    if (result != 0) {
        Serial.println("[IMU] 运动检测配置失败");
        return false;
    }
    
    // 启用运动检测中断
    bool success = false;
    if (intPin == 1) {
        success = qmi.enableMotionDetect(SensorQMI8658::INTERRUPT_PIN_1);
    } else if (intPin == 2) {
        success = qmi.enableMotionDetect(SensorQMI8658::INTERRUPT_PIN_2);
    } else {
        Serial.println("[IMU] 不支持的中断引脚，尝试使用INT1");
        success = qmi.enableMotionDetect(SensorQMI8658::INTERRUPT_PIN_1);
    }
    
    if (success) {
        Serial.printf("[IMU] 运动检测已启用，中断引脚: GPIO%d, 阈值: %.3fg\n", intPin, motionThreshold);
    } else {
        Serial.println("[IMU] 启用运动检测中断失败");
        return false;
    }
    
    // 验证中断引脚状态
    int pinValue = digitalRead(intPin);
    Serial.printf("[IMU] 当前中断引脚状态: %s\n", pinValue ? "HIGH" : "LOW");
    
    return success;
}

void IMU::disableMotionDetection()
{
    if (motionIntPin != -1) {
        // 禁用运动检测中断
        bool success = qmi.disableMotionDetect();
        
        if (success) {
            Serial.printf("[IMU] 已禁用运动检测，中断引脚: GPIO%d\n", motionIntPin);
        } else {
            Serial.println("[IMU] 禁用运动检测失败");
        }
        
        motionIntPin = -1;
    }
}
