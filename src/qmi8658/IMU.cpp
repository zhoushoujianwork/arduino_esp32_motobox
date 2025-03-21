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
    
    // 设置中断引脚为输入模式，带上拉电阻
    pinMode(intPin, INPUT_PULLUP);
    
    // 直接配置QMI8658寄存器来实现运动检测
    // 1. 配置中断控制寄存器
    Wire.beginTransmission(QMI8658_L_SLAVE_ADDRESS);
    Wire.write(0x0E); // INT1_CTRL寄存器地址
    Wire.write(0x20); // 使能ACC任何方向的活动检测中断
    Wire.endTransmission();
    
    // 2. 配置中断引脚配置寄存器（高电平有效，推挽输出）
    Wire.beginTransmission(QMI8658_L_SLAVE_ADDRESS);
    Wire.write(0x0F); // INT_PIN_CONFIG寄存器地址
    Wire.write(0x04); // INT1为推挽输出，高电平有效
    Wire.endTransmission();
    
    // 3. 配置活动检测阈值
    uint8_t thresholdValue = uint8_t(threshold * 32); // 将g值转换为寄存器值
    Wire.beginTransmission(QMI8658_L_SLAVE_ADDRESS);
    Wire.write(0x10); // 活动检测阈值寄存器地址
    Wire.write(thresholdValue); // 设置阈值
    Wire.endTransmission();
    
    // 4. 使能活动检测
    Wire.beginTransmission(QMI8658_L_SLAVE_ADDRESS);
    Wire.write(0x12); // 活动检测控制寄存器
    Wire.write(0x01); // 使能活动检测
    Wire.endTransmission();
    
    Serial.printf("Motion detection enabled on pin %d with threshold %.2f\n", intPin, threshold);
    return true;
}

void IMU::disableMotionDetection()
{
    if (motionIntPin != -1) {
        // 关闭活动检测
        Wire.beginTransmission(QMI8658_L_SLAVE_ADDRESS);
        Wire.write(0x12); // 活动检测控制寄存器
        Wire.write(0x00); // 禁用活动检测
        Wire.endTransmission();
        
        // 关闭中断
        Wire.beginTransmission(QMI8658_L_SLAVE_ADDRESS);
        Wire.write(0x0E); // INT1_CTRL寄存器地址
        Wire.write(0x00); // 禁用所有中断
        Wire.endTransmission();
        
        Serial.println("Motion detection disabled");
        motionIntPin = -1;
    }
}
