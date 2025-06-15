#ifndef BLE_CLIENT_H
#define BLE_CLIENT_H
#include "config.h"
#include <NimBLEDevice.h>
#include "device.h"
#include "gps/GPS.h"
#include "qmi8658/IMU.h"
#include "compass/Compass.h"
#include <algorithm> // 为了使用 std::transform

class BLEC
{
public:
    BLEC();
    void loop();
    void begin();
};

#ifdef BLE_CLIENT
extern BLEC bc;
#endif

#endif