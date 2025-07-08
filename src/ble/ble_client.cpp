#include "ble_client.h"
#include "../gps/GPSData.h"

void scanEndedCB(NimBLEScanResults results);

static NimBLEAdvertisedDevice *myDevice;
static NimBLERemoteCharacteristic *pRemoteCharacteristic;
static NimBLERemoteCharacteristic *pRemoteIMUCharacteristic;
static NimBLERemoteCharacteristic *pRemoteGPSCharacteristic;
BLEC bc;
static bool doConnect = false;
static bool connected = false;
static boolean doScan = false;

static uint32_t scanTime = 0; /** 0 = scan forever */

/** Define a class to handle the callbacks when advertisments are received */
class AdvertisedDeviceCallbacks : public NimBLEAdvertisedDeviceCallbacks
{

    void onResult(NimBLEAdvertisedDevice *advertisedDevice)
    {
        Serial.print("Advertised Device found: ");
        Serial.println(advertisedDevice->toString().c_str());
        if (advertisedDevice->isAdvertisingService(NimBLEUUID(SERVICE_UUID)))
        {
            Serial.println("Found Our Service");
            /** stop scan before connecting */
            NimBLEDevice::getScan()->stop();
            /** Save the device reference in a global for the client to use*/
            myDevice = advertisedDevice;
            /** Ready to connect now */
            doConnect = true;
        }
    };
};

/** Notification / Indication receiving handler callback */
void notifyCB(NimBLERemoteCharacteristic *pRemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify)
{
    if (pRemoteCharacteristic == nullptr || pData == nullptr)
    {
        Serial.println("Invalid notification data or characteristic");
        return;
    }

    // 获取特征值的UUID并转换为大写
    std::string charUUID = pRemoteCharacteristic->getUUID().toString();
    // 转换为大写
    std::transform(charUUID.begin(), charUUID.end(), charUUID.begin(), ::toupper);

    // 现在可以直接比较，因为两个字符串都是大写的
    if (charUUID == DEVICE_CHAR_UUID)
    {
        // 确保数据长度正确
        if (length == sizeof(device_state_t))
        {
            // 使用安全的内存拷贝
            device_state_t deviceState;
            memcpy(&deviceState, pData, sizeof(device_state_t));
            set_device_state(&deviceState);
            print_device_info();
        }
    }
}

/** Callback to process the results of the last scan or restart it */
// 扫描结束回调
void scanEndedCB(NimBLEScanResults results)
{
    Serial.println("Scan Ended");
    if (doConnect)
    {
        // 连接成功后停止扫描
        NimBLEDevice::getScan()->stop();
    }
}

/**  None of these are required as they will be handled by the library with defaults. **
 **                       Remove as you see fit for your needs                        */
class ClientCallbacks : public NimBLEClientCallbacks
{
    void onConnect(NimBLEClient *pClient)
    {
        Serial.println("Connected");
        connected = true;
        doScan = false;
        /** After connection we should change the parameters if we don't need fast response times.
         *  These settings are 150ms interval, 0 latency, 450ms timout.
         *  Timeout should be a multiple of the interval, minimum is 100ms.
         *  I find a multiple of 3-5 * the interval works best for quick response/reconnect.
         *  Min interval: 120 * 1.25ms = 150, Max interval: 120 * 1.25ms = 150, 0 latency, 60 * 10ms = 600ms timeout
         */
        // pClient->updateConnParams(120, 120, 0, 60);
    };

    void onDisconnect(NimBLEClient *pClient)
    {
        Serial.print(pClient->getPeerAddress().toString().c_str());
        Serial.println(" Disconnected - Starting scan");
        connected = false;
        doScan = true;
    };

    /** Called when the peripheral requests a change to the connection parameters.
     *  Return true to accept and apply them or false to reject and keep
     *  the currently used parameters. Default will return true.
     */
    bool onConnParamsUpdateRequest(NimBLEClient *pClient, const ble_gap_upd_params *params)
    {
        if (params->itvl_min < 24)
        { /** 1.25ms units */
            return false;
        }
        else if (params->itvl_max > 40)
        { /** 1.25ms units */
            return false;
        }
        else if (params->latency > 2)
        { /** Number of intervals allowed to skip */
            return false;
        }
        else if (params->supervision_timeout > 100)
        { /** 10ms units */
            return false;
        }

        return true;
    };

    /********************* Security handled here **********************
    ****** Note: these are the same return values as defaults ********/
    uint32_t onPassKeyRequest()
    {
        Serial.println("Client Passkey Request");
        /** return the passkey to send to the server */
        return 123456;
    };

    bool onConfirmPIN(uint32_t pass_key)
    {
        Serial.print("The passkey YES/NO number: ");
        Serial.println(pass_key);
        /** Return false if passkeys don't match. */
        return true;
    };

    /** Pairing process complete, we can check the results in ble_gap_conn_desc */
    void onAuthenticationComplete(ble_gap_conn_desc *desc)
    {
        if (!desc->sec_state.encrypted)
        {
            Serial.println("Encrypt connection failed - disconnecting");
            /** Find the client with the connection handle provided in desc */
            NimBLEDevice::getClientByID(desc->conn_handle)->disconnect();
            return;
        }
    };
};

static ClientCallbacks clientCB;

/** Handles the provisioning of clients and connects / interfaces with the server */
bool connectToServer()
{
    NimBLEClient *pClient = nullptr;
    /** No client to reuse? Create a new one. */
    if (!pClient)
    {
        if (NimBLEDevice::getClientListSize() >= NIMBLE_MAX_CONNECTIONS)
        {
            Serial.println("Max clients reached - no more connections available");
            return false;
        }

        pClient = NimBLEDevice::createClient();

        Serial.println("New client created");

        pClient->setClientCallbacks(&clientCB, false);
        /** Set initial connection parameters: These settings are 15ms interval, 0 latency, 120ms timout.
         *  These settings are safe for 3 clients to connect reliably, can go faster if you have less
         *  connections. Timeout should be a multiple of the interval, minimum is 100ms.
         *  Min interval: 12 * 1.25ms = 15, Max interval: 12 * 1.25ms = 15, 0 latency, 51 * 10ms = 510ms timeout
         */
        pClient->setConnectionParams(12, 12, 0, 51);
        /** Set how long we are willing to wait for the connection to complete (seconds), default is 30. */
        pClient->setConnectTimeout(5);

        if (!pClient->connect(myDevice))
        {
            /** Created a client but failed to connect, don't need to keep it as it has no data */
            NimBLEDevice::deleteClient(pClient);
            Serial.println("Failed to connect, deleted client");
            return false;
        }
    }

    if (!pClient->isConnected())
    {
        if (!pClient->connect(myDevice))
        {
            Serial.println("Failed to connect");
            return false;
        }
    }

    Serial.print("Connected to: ");
    Serial.println(pClient->getPeerAddress().toString().c_str());
    Serial.print("RSSI: ");
    Serial.println(pClient->getRssi());

    /** 连接成功后立即读取特征值 */
    /** Now we can read/write/subscribe the charateristics of the services we are interested in */
    NimBLERemoteService *pSvc = nullptr;
    NimBLERemoteCharacteristic *pChr = nullptr;
    NimBLERemoteDescriptor *pDsc = nullptr;

    pSvc = pClient->getService(SERVICE_UUID);
    if (pSvc)
    {
        // 设备信息特征值
        pRemoteCharacteristic = pSvc->getCharacteristic(DEVICE_CHAR_UUID);
        if (pRemoteCharacteristic == nullptr)
            return false;

        if (pRemoteCharacteristic->canNotify())
        {
            if (pRemoteCharacteristic->subscribe(true, notifyCB))
            {
                Serial.println("Device characteristic notification subscribed successfully");
            }
            else
            {
                Serial.println("Failed to subscribe to device characteristic notification");
                return false;
            }
        }

        // IMU数据特征值
        pRemoteIMUCharacteristic = pSvc->getCharacteristic(IMU_CHAR_UUID);
        if (pRemoteIMUCharacteristic == nullptr)
            return false;

        // GPS数据特征值
       pRemoteGPSCharacteristic = pSvc->getCharacteristic(GPS_CHAR_UUID);
        if (pRemoteGPSCharacteristic == nullptr)
            return false;
    }
    else
    {
        Serial.printf("Service %s not found.\n", SERVICE_UUID);
        return false;
    }

    Serial.println("Connected successfully!");
    return true;
}

BLEC::BLEC()
{
    
}

void BLEC::begin()
{
   Serial.println("Starting NimBLE Client");
    connected = false;
    doConnect = false;
    doScan = false;
    scanTime = 0;

    /** Initialize NimBLE, no device name spcified as we are not advertising */
    NimBLEDevice::init(BLE_NAME);

    /** Set the IO capabilities of the device, each option will trigger a different pairing method.
     *  BLE_HS_IO_KEYBOARD_ONLY    - Passkey pairing
     *  BLE_HS_IO_DISPLAY_YESNO   - Numeric comparison pairing
     *  BLE_HS_IO_NO_INPUT_OUTPUT - DEFAULT setting - just works pairing
     */
    // NimBLEDevice::setSecurityIOCap(BLE_HS_IO_KEYBOARD_ONLY); // use passkey
    // NimBLEDevice::setSecurityIOCap(BLE_HS_IO_DISPLAY_YESNO); //use numeric comparison

    /** 2 different ways to set security - both calls achieve the same result.
     *  no bonding, no man in the middle protection, secure connections.
     *
     *  These are the default values, only shown here for demonstration.
     */
    // NimBLEDevice::setSecurityAuth(false, false, true);
    NimBLEDevice::setSecurityAuth(/*BLE_SM_PAIR_AUTHREQ_BOND | BLE_SM_PAIR_AUTHREQ_MITM |*/ BLE_SM_PAIR_AUTHREQ_SC);

    /** Optional: set the transmit power, default is 3db */
#ifdef ESP_PLATFORM
    NimBLEDevice::setPower(ESP_PWR_LVL_P9); /** +9db */
#else
    NimBLEDevice::setPower(9); /** +9db */
#endif

    /** Optional: set any devices you don't want to get advertisments from */
    // NimBLEDevice::addIgnored(NimBLEAddress ("aa:bb:cc:dd:ee:ff"));

    /** create new scan */
    NimBLEScan *pScan = NimBLEDevice::getScan();

    /** create a callback that gets called when advertisers are found */
    pScan->setAdvertisedDeviceCallbacks(new AdvertisedDeviceCallbacks());

    /** Set scan interval (how often) and window (how long) in milliseconds */
    pScan->setInterval(500);
    pScan->setWindow(50);

    /** Active scan will gather scan response data from advertisers
     *  but will use more energy from both devices
     */
    pScan->setActiveScan(false);
    /** Start scanning for advertisers for the scan time specified (in seconds) 0 = forever
     *  Optional callback for when scanning stops.
     */
    pScan->start(scanTime, scanEndedCB);
    Serial.println("NimBLE Client started");
}

void BLEC::loop()
{
    // Serial.printf("doConnect %d connected %d doScan %d\n", doConnect, connected, doScan);
    /** Loop here until we find a device we want to connect to */
    if (doConnect)
    {
        Serial.println("Connecting....");
        /** Found a device we want to connect to, do it now */
        if (connectToServer())
        {
            Serial.println("Success! we should now be getting notifications, scanning for more!");
            connected = true;
            doConnect = false;
            device_state.bleConnected = true;
        }
        else
        {
            device_state.bleConnected = false;
            Serial.println("Failed to connect, starting scan");
            doScan = true;
            connected = false;
        }
    }

    // Serial.printf("connected: %d\n", connected);
    if (connected)
    {
        // 添加额外的空指针检查
        if (pRemoteGPSCharacteristic->canRead())
        {
            try
            {
                std::string value = pRemoteGPSCharacteristic->readValue();
                if (value.length() == sizeof(gps_data_t))
                {
                    memcpy(&gps_data, value.data(), sizeof(gps_data));
                }
            }
            catch (const std::exception &e)
            {
                Serial.printf("Error reading GPS characteristic: %s\n", e.what());
            }
        }

        if (pRemoteIMUCharacteristic->canRead())
        {
            try
            {
                std::string value = pRemoteIMUCharacteristic->readValue();
                if (value.length() == sizeof(imu_data))
                {
                    memcpy(&imu_data, value.data(), sizeof(imu_data));
                }
                else
                {
                    Serial.printf("IMU data length error: %d\n", value.length());
                }
            }
            catch (const std::exception &e)
            {
                Serial.printf("Error reading IMU characteristic: %s\n", e.what());
            }
        }
        doConnect = false;
    }
    else if (doScan)
    {
        Serial.println("Starting scan");
        NimBLEDevice::getScan()->start(scanTime, scanEndedCB);
        doScan = false;
        device_state.bleConnected = false;
    }
}
