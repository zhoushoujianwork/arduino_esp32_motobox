#include "GPS.h"
#include <TinyGPS++.h>

// \r\n 是回车换行符的组合
// \r (CR, Carriage Return) - 回车，将光标移动到行首
// \n (LF, Line Feed) - 换行，将光标移动到下一行
// 在串行通信中,\r\n的组合是标准的行结束符
// NMEA协议规定每条GPS语句都必须以\r\n结尾

/*
GGA：时间、位置、卫星数量
GSA：GPS接收机操作模式，定位使用的卫星，DOP值，定位状态
GSV：可见GPS卫星信息、仰角、方位角、信噪比
RMC：时间、日期、位置、速度
VTG：地面速度信息

$PCAS04,3*1A 北斗和 GPS 双模
$PCAS04,1*18 单 GPS 工作模式
$PCAS04,2*1B 单北斗工作模式

// $PCAS03,nGGA,nGLL,nGSA,nGSV,nRMC,nVTG,nZDA,nANT,nDHV,nLPS,res1,res2,nUTC,nGST,res3,res4,res5,nTIM*CS<CR><LF>
// GGA 输出频率，语句输出频率是以定位更新率为基准的，n（0~9）表示每 n 次定位输出一次，0 表示不输出该语句，空则保持原有配置。
// $PCAS03,1,1,1,1,1,1,1,1,0,0,,,1,1,,,,1*33
// RMC,ZDA,nVTG 打开其他都关闭 $PCAS03,0,0,0,0,1,1,1,0,0,0,,,0,0,,,,0*33
// $PCAS06,0*1B 查询产品信息

/*
检查 GPS 天线 状态
$GPTXT,01,01,01,ANTENNA OPEN*25
表示天线状态（开路）
$GPTXT,01,01,01,ANTENNA OK*35
表示天线状态（良好）
$GPTXT,01,01,01,ANTENNA SHORT*63
表示天线状态（短路）
*/

// 定义常用的GPS波特率数组
const uint32_t BAUD_RATES[] = {9600,19200, 38400, 57600, 115200, 230400, 460800, 921600};
const int NUM_BAUD_RATES = 8;

#ifdef ENABLE_GPS
GPS gps(GPS_RX_PIN, GPS_TX_PIN);
#endif

gps_data_t gps_data;

GPS::GPS(int rxPin, int txPin) : gpsSerial(rxPin, txPin),
                                 _debug(true),
                                 _foundBaudRate(false),
                                 _lastDebugPrintTime(0)
{
    _debug = false;
    _rxPin = rxPin;
    _txPin = txPin;
    gps_data.gpsHz = 2;
}

void GPS::begin()
{
    Serial.println("[GPS] 开始初始化 txPin:" + String(_txPin) + " rxPin:" + String(_rxPin) + " _foundBaudRate:" + String(_foundBaudRate));

    // 尝试自动检测正确的波特率，直到找到为止
    while (!_foundBaudRate)
    {
        for (int i = 0; i < NUM_BAUD_RATES; i++)
        {
            debugPrint("尝试波特率: " + String(BAUD_RATES[i]));
            uint32_t testBaudRate = BAUD_RATES[i];

            // 设置新的波特率
            gpsSerial.begin(testBaudRate);

            // 清空接收缓冲区
            while (gpsSerial.available())
            {
                gpsSerial.read();
            }

            // 发送查询命令
            String cmd = "$PCAS06,0*1B\r\n"; // 查询产品信息命令
            gpsSerial.print(cmd);
            gpsSerial.flush();

            if (isValidGpsResponse())
            {
                _foundBaudRate = true;
                _currentBaudRate = testBaudRate;
                Serial.printf("[GPS] 找到正确的波特率: %d\n", testBaudRate);
                break;
            }
        }

        if (!_foundBaudRate)
        {
            Serial.println("[GPS] 未找到正确的波特率，重试...");
            delay(1000); // 等待1秒后重试
        }
    }

// 额外设置指定串口速率
#ifdef GPS_BAUDRATE

    if (_currentBaudRate != GPS_BAUDRATE)
    {
        if (setBaudRate(GPS_BAUDRATE))
        {
            Serial.println("[GPS] 设置波特率成功,波特率:" + String(GPS_BAUDRATE));
        }

        // 还是失败，可能是设备不兼容语句无法切换
        if (_currentBaudRate != GPS_BAUDRATE)
        {
            Serial.println("[GPS] 设置波特率失败，设备不兼容语句无法切换，使用默认波特率:" + String(_currentBaudRate));
            setBaudRate(_currentBaudRate);
        }
    }
#endif

    // 设置频率
    while (!setHz(gps_data.gpsHz))
    {
        Serial.println("[GPS] 设置频率失败，重试");
        delay(100);
    }
    Serial.println("[GPS] 设置频率成功,频率:" + String(gps_data.gpsHz));

    // 设置双模 ，1北斗，2GPS，3双模
    while (!sendGpsCommand(buildModeCmd(3), 3, 100))
    {
        Serial.println("[GPS] 设置双模失败，重试");
        delay(100);
    }
    Serial.println("[GPS] 设置双模成功");

    Serial.println("[GPS] 初始化完成");
}

void GPS::loop()
{
    if (_debug)
    {
        printRawData();
    }
    else
    {
        while (gpsSerial.available() > 0)
        {
            char c = (char)gpsSerial.read();
            if (gps.encode(c))
            {
                if (gps.location.isUpdated())
                {
                    // 使用更高效的时间获取方式
                    auto loc = gps.location;
                    gps_data.latitude = loc.lat();
                    gps_data.longitude = loc.lng();
                }

                if (gps.altitude.isUpdated())
                {
                    auto alt = gps.altitude;
                    gps_data.altitude = alt.meters();
                }

                if (gps.course.isUpdated())
                {
                    auto course = gps.course;
                    gps_data.heading = course.deg();
                }

                if (gps.time.isUpdated())
                {
                    auto time = gps.time;
                    gps_data.hour = time.hour();
                    gps_data.minute = time.minute();
                    gps_data.second = time.second();
                    gps_data.centisecond = time.centisecond();
                }

                if (gps.date.isUpdated())
                {
                    auto date = gps.date;
                    gps_data.year = date.year();
                    gps_data.month = date.month();
                    gps_data.day = date.day();
                }

                if (gps.hdop.isUpdated())
                {
                    // 获取
                    auto hdop = gps.hdop;
                    gps_data.hdop = hdop.value();
                }

                if (gps.satellites.isUpdated())
                {
                    auto satellites = gps.satellites;
                    gps_data.satellites = satellites.value();
                    // 超过 3 个表示GPS准备好了
                    if (gps_data.satellites > 3)
                    {
                        device_state.gpsReady = true;
                        loopAutoAdjustHz();
                    }
                    else
                    {
                        device_state.gpsReady = false;
                    }
                }

                if (gps.speed.isUpdated())
                {
                    auto speed = gps.speed;
                    gps_data.speed = speed.kmph();
                    loopDistance();
                }
            }
        }
    }
}

void GPS::printRawData()
{
    // 打印缓冲区中的数据
    while (gpsSerial.available())
    {
        char c = gpsSerial.read();
        Serial.print(c);
    }
}

/**
 * 切换频率 在 1，2，5，10 Hz 之间切换
 * @return 切换后的频率
 */
int GPS::changeHz()
{
    // 计算下一个目标频率
    int nextHz = gps_data.gpsHz == 1 ? 2 : gps_data.gpsHz == 2 ? 5
                                       : gps_data.gpsHz == 5   ? 10
                                                               : 1;

    // 尝试设置新频率
    if (setHz(nextHz))
    {
        Serial.println("[GPS] 频率切换成功: " + String(nextHz) + "Hz");
        gps_data.gpsHz = nextHz; // 只有在设置成功时才更新频率值
    }
    else
    {
        Serial.println("[GPS] 频率切换失败，保持当前频率: " + String(gps_data.gpsHz) + "Hz");
    }

    return gps_data.gpsHz; // 返回实际的频率值
}

/**
 * NMEA 语法设置波特率
 * @param baudRate 目标波特率 (支持 9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600)
 * @return bool 设置是否成功
 */
bool GPS::setBaudRate(int baudRate)
{
    String cmd = buildBaudrateCmd(baudRate);
    if (cmd.isEmpty()) {
        Serial.println("[GPS] 不支持的波特率: " + String(baudRate));
        return false;
    }

    // 先执行一次命令
    sendGpsCommand(cmd, 3, 100);
    
    // 等待GPS模块稳定
    delay(500);
    
    // 重新初始化串口
    gpsSerial.end();
    delay(100);
    gpsSerial.begin(baudRate);
    delay(100);

    // 清空接收缓冲区
    while (gpsSerial.available()) {
        gpsSerial.read();
    }

    // 发送测试命令并验证响应
    if (sendGpsCommand("$PCAS06,0*1B\r\n", 3, 100)) {
        _currentBaudRate = baudRate;
        Serial.println("[GPS] 切换波特率成功: " + String(baudRate));
        return true;
    }

    Serial.println("[GPS] 切换波特率失败: " + String(baudRate));
    return false;
}

/**
 * NMEA 语法设置频率
 * @param hz 目标频率 (支持 1, 2, 5, 10 Hz)
 * @return bool 设置是否成功
 */
bool GPS::setHz(int hz)
{
    String cmd = buildUpdateRateCmd(hz);
    return sendGpsCommand(cmd, 3, 100);
}

// GPS类的实现
String GPS::calculateChecksum(const String &cmd)
{
    // 移除$符号，只计算$后面的部分
    String data = cmd.substring(1);
    // 如果命令包含*号，去掉*号及其后面的内容
    int starPos = data.indexOf('*');
    if (starPos != -1)
    {
        data = data.substring(0, starPos);
    }

    byte checksum = 0;
    // 计算异或校验和
    for (unsigned int i = 0; i < data.length(); i++)
    {
        checksum ^= data.charAt(i);
    }

    // 转换为十六进制字符串，确保两位
    char checksumStr[3];
    sprintf(checksumStr, "%02X", checksum);
    return String(checksumStr);
}

String GPS::buildBaudrateCmd(int baudRate)
{
    String cmd;
    switch (baudRate)
    {
    case 4800:
        cmd = "$PCAS01,0";
        break;
    case 9600:
        cmd = "$PCAS01,1";
        break;
    case 19200:
        cmd = "$PCAS01,2";
        break;
    case 38400:
        cmd = "$PCAS01,3";
        break;
    case 57600:
        cmd = "$PCAS01,4";
        break;
    case 115200:
        cmd = "$PCAS01,5";
        break;
    case 230400:
        cmd = "$PCAS01,6";
        break;
    case 460800:
        cmd = "$PCAS01,7";
        break;
    case 921600:
        cmd = "$PCAS01,8";
        break;
    default:
        return ""; // 不支持的波特率返回空字符串
    }

    // 计算并添加校验和
    cmd += "*" + calculateChecksum(cmd);
    return cmd + "\r\n";
}

String GPS::buildModeCmd(int mode)
{
    String cmd;
    switch (mode)
    {
    case 1:
        cmd = "$PCAS04,1";
        break; // 单GPS模式
    case 2:
        cmd = "$PCAS04,2";
        break; // 单北斗模式
    case 3:
        cmd = "$PCAS04,3";
        break; // 北斗和GPS双模
    default:
        return ""; // 不支持的模式返回空字符串
    }

    // 计算并添加校验和
    cmd += "*" + calculateChecksum(cmd);
    return cmd + "\r\n";
}

String GPS::buildUpdateRateCmd(int ms)
{
    if (ms <= 0)
        return "";

    String cmd = "$PCAS02," + String(ms);

    // 计算并添加校验和
    cmd += "*" + calculateChecksum(cmd);
    return cmd + "\r\n";
}

// 优化后的命令发送函数，判断是否收到有效的响应数据
bool GPS::sendGpsCommand(const String &cmd, int retries, int retryDelay)
{
    if (cmd.isEmpty())
    {
        Serial.println("[GPS] 发送空命令");
        return false;
    }

    Serial.print("[GPS] 发送命令 -> ");
    Serial.print(cmd);

    // 清空接收缓冲区
    while (gpsSerial.available())
    {
        gpsSerial.read();
    }

    bool success = false;
    for (int i = 0; i < retries && !success; i++)
    {
        if (i > 0)
        {
            Serial.printf("[GPS] 重试发送命令 (%d/%d)\n", i + 1, retries);
            delay(100); // 重试前额外延时
        }

        gpsSerial.print(cmd);
        gpsSerial.flush();
        delay(retryDelay * 2);

        // 直接调用isValidGpsResponse判断
        if (isValidGpsResponse())
        {
            success = true;
        }
    }

    if (success)
    {
        Serial.println("[GPS] 命令发送成功");
    }
    else
    {
        Serial.println("[GPS] 命令发送失败");
    }
    return success;
}

/**
 * 从串口读取并判断GPS数据是否有效
 * @return bool 数据是否有效
 */
bool GPS::isValidGpsResponse()
{
    unsigned long startTime = millis();
    String response = "";
    bool foundValidSentence = false;
    int totalBytes = 0; // 记录接收到的总字节数

    // 等待最多2000ms检查是否有数据返回
    while (millis() - startTime < 2000) // 增加到2000ms
    {
        if (gpsSerial.available())
        {
            char c = gpsSerial.read();
            response += c;
            totalBytes++;

            // 检查是否收到完整的NMEA语句
            if (c == '\n')
            {
                // 检查是否是有效的NMEA语句
                if (response.indexOf('$') != -1 &&
                    response.indexOf('*') != -1 &&
                    response.length() >= 10) // 确保语句长度合理
                {
                    // 验证校验和
                    int starPos = response.indexOf('*');
                    if (starPos != -1 && starPos + 3 <= response.length())
                    {
                        String data = response.substring(1, starPos);
                        String checksum = response.substring(starPos + 1, starPos + 3);

                        // 计算校验和
                        byte calcChecksum = 0;
                        for (unsigned int i = 0; i < data.length(); i++)
                        {
                            calcChecksum ^= data.charAt(i);
                        }

                        // 转换为十六进制字符串
                        char checksumStr[3];
                        sprintf(checksumStr, "%02X", calcChecksum);

                        // 比较校验和
                        if (String(checksumStr) == checksum)
                        {
                            foundValidSentence = true;
                            debugPrint("[GPS] 收到有效响应: " + response);
                            debugPrint("[GPS] 当前波特率: " + String(_currentBaudRate) +
                                       ", 接收字节数: " + String(totalBytes) +
                                       ", 语句长度: " + String(response.length()));
                            break;
                        }
                        else
                        {
                            debugPrint("[GPS] 校验和错误 - 计算值: " + String(checksumStr) +
                                       ", 接收值: " + checksum);
                        }
                    }
                }
                else
                {
                    debugPrint("[GPS] 无效NMEA语句: " + response);
                }
                response = ""; // 清空响应缓冲区
            }
        }
        delay(5); // 减少CPU使用率
    }

    if (!foundValidSentence)
    {
        debugPrint("[GPS] 未找到有效响应 - 波特率: " + String(_currentBaudRate) +
                   ", 总接收字节数: " + String(totalBytes));
    }

    return foundValidSentence;
}

/*
 * 依据卫星数量自动调节频率，20颗星以上10Hz，10-20颗星5Hz，10颗星以下2Hz， 5s 检查执行一次
 * @param satellites 卫星数量
 * @return 调节后的频率
 */
void GPS::loopAutoAdjustHz()
{
    if (millis() - lastAutoAdjustHzTime >= 5000 && device_state.gpsReady)
    {
        autoAdjustHz(gps_data.satellites);
        lastAutoAdjustHzTime = millis();
    }
}
int GPS::autoAdjustHz(uint8_t satellites)
{
    // 根据卫星数量计算目标频率
    int targetHz = satellites > 20 ? 10 : satellites > 10 ? 5
                                                          : 2;

    // 波特率在 9600 只能切换 1,2,5 hz
    if (_currentBaudRate == 9600)
    {
        // 如果目标频率是10Hz，则降级到5Hz
        if (targetHz == 10)
        {
            targetHz = 5;
        }
    }

    if (targetHz == gps_data.gpsHz)
    {
        // 符合预期直接返回
        return targetHz;
    }

    if (setHz(targetHz))
    {
        Serial.println("[GPS] 自动调节频率成功: " + String(targetHz) + "Hz");
        gps_data.gpsHz = targetHz;
        return targetHz;
    }
    else
    {
        Serial.println("[GPS] 自动调节频率失败，保持当前频率: " + String(gps_data.gpsHz) + "Hz");
        return gps_data.gpsHz;
    }
}

/*
外部按键切换波特率
*/
int GPS::changeBaudRate()
{
    // 获取当前波特率，并切换到下一个波特率
    int currentBaudRate = _currentBaudRate;
    int nextBaudRate = currentBaudRate == 9600 ? 19200 : currentBaudRate == 19200 ? 38400
                                                     : currentBaudRate == 38400   ? 57600
                                                     : currentBaudRate == 57600   ? 115200
                                                     : currentBaudRate == 115200  ? 230400
                                                     : currentBaudRate == 230400  ? 460800
                                                     : currentBaudRate == 460800  ? 921600
                                                                                  : 9600;
    if (setBaudRate(nextBaudRate))
    {
        _currentBaudRate = nextBaudRate;
        return nextBaudRate;
    }
    return currentBaudRate;
}

/**
 * @brief 打印GPS数据
 */
void GPS::printGpsData()
{
    char timeStr[30];
    sprintf(timeStr, "%04d-%02d-%02d %02d:%02d:%02d",
            gps_data.year, gps_data.month, gps_data.day,
            gps_data.hour, gps_data.minute, gps_data.second);

    Serial.println("gps_data: " + String(timeStr) + ", " +
                   String(gps_data.latitude) + ", " +
                   String(gps_data.longitude) + ", " +
                   String(gps_data.altitude) + ", " +
                   String(gps_data.speed) + ", " +
                   String(gps_data.heading) + ", " +
                   String(gps_data.satellites));

    // 添加HDOP状态显示
    const char *hdopStatus = "";
    if (gps_data.hdop == 0)
    {
        hdopStatus = "无数据";
    }
    else if (gps_data.hdop < 1.0)
    {
        hdopStatus = "优秀";
    }
    else if (gps_data.hdop < 2.0)
    {
        hdopStatus = "良好";
    }
    else if (gps_data.hdop < 5.0)
    {
        hdopStatus = "一般";
    }
    else
    {
        hdopStatus = "较差";
    }

    Serial.println("HDOP: " + String(gps_data.hdop, 1) + " (" + hdopStatus + ")");
}

void GPS::loopDistance()
{
    // 获取当前GPS数据
    // 如果没有有效的GPS数据或卫星数量不足，返回当前累积距离
    if (gps_data.satellites < 3)
    {
        return;
    }

    // 获取当前时间
    unsigned long currentTime = millis();

    // 如果是第一次计算，初始化lastDistanceTime
    if (lastDistanceTime == 0)
    {
        lastDistanceTime = currentTime;
        return;
    }

    // 计算时间间隔（秒）
    float timeInterval = (currentTime - lastDistanceTime) / 1000.0;

    // 计算距离增量
    float distanceIncrement = (gps_data.speed / 3600.0) * timeInterval;

    // 累加距离
    totalDistanceKm += distanceIncrement;
    // 更新最后计算时间
    lastDistanceTime = currentTime;
}

String gps_data_to_json(const gps_data_t &data)
{
    // 使用ArduinoJson库将gps_data转换为JSON字符串
    StaticJsonDocument<256> doc;
    doc["lat"] = gps_data.latitude;
    doc["lon"] = gps_data.longitude;
    doc["alt"] = gps_data.altitude;
    doc["speed"] = gps_data.speed;
    doc["satellites"] = gps_data.satellites;
    doc["heading"] = gps_data.heading;
    doc["year"] = gps_data.year;
    doc["month"] = gps_data.month;
    doc["day"] = gps_data.day;
    doc["hour"] = gps_data.hour;
    doc["minute"] = gps_data.minute;
    doc["second"] = gps_data.second;
    doc["hdop"] = gps_data.hdop;

    String jsonString;
    serializeJson(doc, jsonString);
    return jsonString;
}

void GPS::debugPrint(const String &message)
{
    if (_debug)
    {
        Serial.println("[GPS] [debug] " + message);
    }
}