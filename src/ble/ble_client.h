#ifndef BLE_CLIENT_H
#define BLE_CLIENT_H
#include "config.h"
#include <NimBLEDevice.h>
#include "device.h"
#include "imu/qmi8658.h"
#include "compass/Compass.h"
#include <algorithm> // 为了使用 std::transform
#include "Air780EG.h"

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