#ifndef LBS_DATA_H
#define LBS_DATA_H

#include <Arduino.h>
#include "GPSData.h"

// LBS请求状态
enum class LBSState {
    IDLE,           // 空闲状态
    REQUESTING,     // 正在请求
    WAIT_RESPONSE,  // 等待响应
    DONE,          // 完成
    ERROR          // 错误
};

struct lbs_data_t {
    bool valid;
    float latitude;
    float longitude;
    int radius;
    String descr;
    int stat;
    LBSState state;         // 当前状态
    unsigned long lastRequestTime;  // 上次请求时间
    unsigned long requestStartTime; // 请求开始时间
    String operator_name;
    unsigned long timestamp;
};

extern lbs_data_t lbs_data;

// 辅助函数
void reset_lbs_data(lbs_data_t& data);
bool is_lbs_data_valid(const lbs_data_t& data);
void print_lbs_data(const lbs_data_t& data);
gps_data_t convert_lbs_to_gps(const lbs_data_t& lbsData);

#endif // LBS_DATA_H 