#ifndef BLE_CLIENT_H
#define BLE_CLIENT_H
#include "config.h"
#include <NimBLEDevice.h>
#include "device.h"
#include "gps/GPS.h"
#include "qmi8658/IMU.h"

class BLEC
{
public:
    BLEC();
    void setup();
    void loop();

};

#endif