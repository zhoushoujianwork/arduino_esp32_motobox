#include "qmi8658.h"

IMU imu(IMU_INT_PIN);

imu_data_t imu_data;

// 数据回调函数（C风格，用于库的回调）
static void imu_data_callback(imu_data_t& data) {
    extern imu_data_t imu_data;
    extern Device device;
    imu_data = data;
    device.updateIMUData(data.accel_x, data.accel_y, data.accel_z,
                        data.gyro_x, data.gyro_y, data.gyro_z,
                        data.roll, data.pitch, data.yaw);
}

// 状态回调函数（C风格，用于库的回调）
static void imu_status_callback(bool ready) {
    get_device_state()->telemetry.modules.imu_ready = ready;
}

// 转发到库的静态成员
volatile bool IMU::motionInterruptFlag = false;
void IRAM_ATTR IMU::motionISR()
{
    QMI8658IMU::motionISR(); // 转发到库
    IMU::motionInterruptFlag = true;
}

IMU::IMU(int motionIntPin)
    : _imu(motionIntPin, IIC_SDA_PIN, IIC_SCL_PIN, QMI8658_L_SLAVE_ADDRESS)
{
    // 设置数据回调，更新全局imu_data
    _imu.setDataCallback(imu_data_callback);
    
    // 设置状态回调
    _imu.setStatusCallback(imu_status_callback);
}

void IMU::begin()
{
    // 使用共享I2C管理器
    if (!I2CManager::getInstance().isInitialized()) {
        Serial.println("[IMU] ❌ 共享I2C未初始化，请先初始化I2C管理器");
        return;
    }
    
    TwoWire& wire = getSharedWire();
    if (!_imu.begin(wire)) {
        Serial.println("[IMU] 初始化失败，系统将重启");
        delay(1000);
        esp_restart();
    }
}

void IMU::configureMotionDetection(float threshold)
{
    // 注意：库在begin()时已经配置了默认的运动检测
    // 如果需要动态配置，可以通过库的公共接口添加方法
    // 目前先保持兼容性
}

void IMU::disableMotionDetection()
{
    _imu.disableMotionDetection();
}

bool IMU::configureForDeepSleep()
{
    return _imu.configureForDeepSleep();
}

bool IMU::restoreFromDeepSleep()
{
    // 重新初始化I2C总线
    TwoWire& wire = getSharedWire();
    initSharedI2C(IIC_SDA_PIN, IIC_SCL_PIN);
    delay(50);
    
    return _imu.restoreFromDeepSleep();
}

bool IMU::checkWakeOnMotionEvent()
{
    return _imu.checkWakeOnMotionEvent();
}

bool IMU::isMotionDetected()
{
    return _imu.isMotionDetected();
}

void IMU::setAccelPowerMode(uint8_t mode)
{
    _imu.setAccelPowerMode(mode);
}

void IMU::setGyroEnabled(bool enabled)
{
    _imu.setGyroEnabled(enabled);
}

void IMU::loop()
{
    _imu.loop();
    // 数据已通过回调函数更新到imu_data和device
}

bool IMU::detectMotion()
{
    return _imu.detectMotion();
}

void IMU::printImuData()
{
    _imu.printImuData();
}

// 生成精简版IMU数据JSON
String imu_data_to_json(imu_data_t &imu_data)
{
    // 使用库中的函数
    return ::imu_data_to_json(imu_data);
}