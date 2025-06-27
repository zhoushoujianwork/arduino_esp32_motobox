/*
 * GSM模块GNSS功能适配器实现
 */

#include "GSMGNSSAdapter.h"
#include "GPSData.h"

GSMGNSSAdapter::GSMGNSSAdapter(Ml307AtModem& modem) 
    : _modem(modem), _ready(false) {
}