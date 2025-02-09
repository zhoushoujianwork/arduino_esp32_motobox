#include "IMU.h"

#define USE_WIRE

IMU::IMU(int sda, int scl)
{
    this->sda = sda;
    this->scl = scl;
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
            delay(1000);
        }
    }
#else
    if (!qmi.begin(IMU_CS))
    {
        Serial.println("Failed to find QMI8658 - check your wiring!");
        while (1)
        {
            delay(1000);
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
        qmi.getAccelerometer(imu_data.accel_x, imu_data.accel_y, imu_data.accel_z);
        qmi.getGyroscope(imu_data.gyro_x, imu_data.gyro_y, imu_data.gyro_z);

        // 使用加速度计计算姿态角
        float roll_acc = atan2(imu_data.accel_y, imu_data.accel_z) * 180 / M_PI;
        float pitch_acc = atan2(-imu_data.accel_x, sqrt(imu_data.accel_y * imu_data.accel_y + imu_data.accel_z * imu_data.accel_z)) * 180 / M_PI;

        // 实现 roll翻转+180度
        roll_acc += 180;
        // 添加翻转适配逻辑
        if (roll_acc > 180) {
            roll_acc -= 360; // 将roll角翻转到[-180, 180]范围
        } else if (roll_acc < -180) {
            roll_acc += 360; // 将roll角翻转到[-180, 180]范围
        }

        // 使用陀螺仪数据和互补滤波更新姿态角
#define ALPHA 0.7
        imu_data.roll = ALPHA * (imu_data.roll + imu_data.gyro_x * dt) + (1.0 - ALPHA) * roll_acc;
        imu_data.pitch = ALPHA * (imu_data.pitch + imu_data.gyro_y * dt) + (1.0 - ALPHA) * pitch_acc;

        get_imu_data()->temperature = qmi.getTemperature_C();
    }
}

float IMU::getRoll()
{
    return imu_data.roll;
}

float IMU::getPitch()
{
    return imu_data.pitch;
}

imu_data_t *IMU::get_imu_data()
{
    return &imu_data;
}

void IMU::printImuData()
{
    // Serial.println("imu_data: " + String(imu_data.accel_x) + ", " + String(imu_data.accel_y) + ", " + String(imu_data.accel_z) + ", " + String(imu_data.gyro_x) + ", " + String(imu_data.gyro_y) + ", " + String(imu_data.gyro_z) + ", " + String(imu_data.roll) + ", " + String(imu_data.pitch) + ", " + String(imu_data.yaw) + ", " + String(imu_data.temperature));
    // 只打印 pitch 和 roll
    Serial.println("imu_data: pitch: " + String(imu_data.pitch) + ", roll: " + String(imu_data.roll));
}

// 将imu_data_t结构体转换为JSON字符串
String imu_data_to_json(imu_data_t imu_data)
{
    // 使用ArduinoJson库将imu_data转换为JSON字符串
    StaticJsonDocument<200> doc;
    doc["roll"] = imu_data.roll;
    doc["pitch"] = imu_data.pitch;
    doc["yaw"] = imu_data.yaw;
    doc["temperature"] = imu_data.temperature;
    String jsonString;
    serializeJson(doc, jsonString);
    return jsonString;
}