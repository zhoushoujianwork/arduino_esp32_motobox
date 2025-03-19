#ifndef CONFIG_H
#define CONFIG_H

/* Version Information */
#define VERSION "2.0.0"

/*
 * Device Operation Mode Configuration
 * Supported modes:
 * 1. ALL_IN_ONE: Standalone mode without Bluetooth, direct data collection and display
 * 2. SERVER: Master mode - collects sensor data, sends via Bluetooth to client,
 *           and can send data to server via MQTT over WiFi
 * 3. CLIENT: Slave mode - receives data from server via Bluetooth and displays it
 */
#define MODE_ALL_IN_ONE
// #define MODE_SERVER
// #define MODE_CLIENT

/* Common Device Configuration */
#define BLE_NAME            "ESP32-MOTO"
#define SERVICE_UUID        "4FAFC201-1FB5-459E-8FCC-C5C9C331914B"
#define DEVICE_CHAR_UUID    "BEB5483A-36E1-4688-B7F5-EA07361B26A8"
#define GPS_CHAR_UUID       "BEB5483E-36E1-4688-B7F5-EA07361B26A8"
#define IMU_CHAR_UUID       "BEB5483F-36E1-4688-B7F5-EA07361B26A8"

/* Battery Configuration */
#define BAT_MIN_VOLTAGE     2900
#define BAT_MAX_VOLTAGE     3250

/* Display Configuration */
#define TFT_HOR_RES        172
#define TFT_VER_RES        320
#define UI_MAX_SPEED       199    /* Maximum speed in km/h */

/* LED Configuration */
#define LED_BLINK_INTERVAL 100

/* IMU Configuration */
#define IMU_MAX_D          68     /* Marquis GP retention */

/* GPS Configuration */
#define GPS_DEFAULT_BAUDRATE 9600
#define GPS_HZ              2      /* Supported: 1, 2, 5, 10 Hz */

/* MQTT Configuration */
#define MQTT_SERVER        "mq-hub.daboluo.cc"
#define MQTT_PORT         1883
#define MQTT_USER         ""
#define MQTT_PASSWORD     ""

/* MQTT Topics */
#define MQTT_TOPIC_DEVICE_INFO  String("vehicle/v1/") + device.get_device_id() + "/device/info"
#define MQTT_TOPIC_GPS         String("vehicle/v1/") + device.get_device_id() + "/gps/position"
#define MQTT_TOPIC_IMU         String("vehicle/v1/") + device.get_device_id() + "/imu/gyro"

/* MQTT Intervals (in milliseconds) */
#define MQTT_DEVICE_INFO_INTERVAL  5000
#define MQTT_IMU_PUBLISH_INTERVAL   200
#define MQTT_BAT_PRINT_INTERVAL   10000

#ifdef MODE_ALL_IN_ONE
    /* Pin Configuration for ALL_IN_ONE Mode */
    #define BTN_PIN         39
    #define BAT_PIN         20
    #define IMU_SDA_PIN     42
    #define IMU_SCL_PIN      2
    #define LED_PIN          8

    /* TFT Display Pins */
    #define TFT_CS           6
    #define TFT_DC           4
    #define TFT_MOSI        15
    #define TFT_SCLK         5
    #define TFT_RST          7
    #define TFT_BL          16
#endif

#ifdef MODE_SERVER
    #define BLE_SERVER              /* Enable server mode */
    
    /* Pin Configuration for SERVER Mode */
    #define IMU_SDA_PIN     42
    #define IMU_SCL_PIN      2
    #define BTN_PIN         39
    #define BAT_PIN          7
    #define LED_PIN          8
    
    /* GPS UART Pins */
    #define GPS_RX_PIN      12
    #define GPS_TX_PIN      11
#endif

#ifdef MODE_CLIENT
    #define BLE_CLIENT              /* Enable client mode */
    
    /* Pin Configuration for CLIENT Mode */
    #define BAT_PIN         14
    #define LED_PIN          3

    /* TFT Display Pins */
    #define TFT_CS          15
    #define TFT_DC           2
    #define TFT_MOSI        19
    #define TFT_SCLK        18
    #define TFT_RST          4
    #define TFT_BL           5
#endif

#endif /* CONFIG_H */
