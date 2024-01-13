#include <Arduino.h>
#ifndef AM2H_Core_Constants_h
#define AM2H_Core_Constants_h

// Logger settings
constexpr uint8_t LOG_LENGTH{40}; // how many entries?
constexpr uint8_t LOG_CALLER_LENGTH{16}; // Stringlength of the "caller"
constexpr uint8_t LOG_MESSAGE_LENGTH{64}; // Stringlength of the "info text"

// Pin Setup
constexpr uint8_t CORE_SCL{0}; // I2C SCL must be "0" for ESP8266
constexpr uint8_t CORE_SDA{2};  // I2C SDA must be "2" for ESP8266

constexpr uint8_t CORE_ISR_PIN{1}; // Interrupt
constexpr uint32_t CORE_ISR_DEBOUNCE{50}; // debounce time in ms

constexpr uint8_t CORE_TX{1}; // Serial TX only active when SERIALDEBUG is enabled
constexpr uint8_t CORE_RX{3};  // Serial RX only active when SERIALDEBUG is enabled

constexpr uint8_t CORE_STATUS_LED{2}; // Internal LED (blue)
constexpr uint8_t CORE_DQ{3};  // Onewire

// Icounter
constexpr uint32_t ICOUNTER_MIN_IMPULSE_TIME_MS{30};
constexpr uint32_t ICOUNTER_MIN_IMPULSE_RATE_MS{500};

// Pcf8574
constexpr uint32_t PCF8574_MIN_RATE_MS{200}; // must be bigger than PCF8574_WAIT_READ_MS
constexpr uint32_t PCF8574_WAIT_READ_MS{15}; // must be smaller than PCF8574_MIN_RATE_MS

// Counter
constexpr uint32_t ONE_MINUTE {1000*60};
constexpr uint8_t COUNTER_MAX_BUFFER {50};
constexpr uint32_t COUNTER_LAST_DURATION {ONE_MINUTE *30};

// Mqtt and Device
namespace MQTT {
    String const DEFAULT_SERVER {"192.168.178.10"};
    constexpr int DEFAULT_PORT {1883};
    String const DEVICE {"am2hDevice"};
    String const NAMESPACE {"myLocation"};
}

namespace DebugLogger {
    constexpr uint32_t LOGLEN{6000};
    constexpr uint32_t SHORTBY{1000};
    constexpr bool ERROR{false};
    constexpr bool INFO{true};
}

// I2C config
constexpr uint8_t TCAADDR{0x70}; // Multiplexer address

// Connection Status
constexpr uint8_t DEV_RESTART_PENDING   {0x00};
constexpr uint8_t CONN_UNKNOWN          {0x01};
constexpr uint8_t WLAN_AP_PROV          {0x02};
constexpr uint8_t WLAN_CONNECTED        {0x03};
constexpr uint8_t MQTT_CLIENT_CONNECTED {0x04};
constexpr uint8_t DEV_CONFIGURED        {0x05};

// API Updates
constexpr uint8_t NO_UPDATE_REQUIRED        {0b00000001};
constexpr uint8_t COMMIT_TO_EEPROM_REQUIRED {0b00000010};
constexpr uint8_t WLAN_RESET_REQUIRED       {0b00000100};
constexpr uint8_t MQTT_RESET_REQUIRED       {0b00001000};
constexpr uint8_t MQTT_UPDATE_REQUIRED      {0b00010000};
// #define MQTT_PUBLISH_MESSAGE      0b00100000
constexpr uint8_t ESP_RESET_REQUIRED        {0b10000000};

// MQTT Updates
int constexpr WLAN_TIMEOUT{20}; // Wlan timeout in s
// #define ENCODING_HTML "text/html; charset=UTF-8"

// HTTP Api
char constexpr ENCODING_PLAIN[] {"text/plain; charset=UTF-8"};
char constexpr ENCODING_HTML[] {"text/html; charset=utf-8"};
char constexpr ENCODING_JSON[] {"application/json; charset=utf-8"};
char constexpr HTTP_API_V1_SET[] {"/api/v1/set"};
char constexpr HTTP_API_V1_GET[] {"/api/v1/get"};
char constexpr HTTP_API_V1_RESTART[]  {"/api/v1/restart"};
char constexpr HTTP_API_V1_STATUS[] {"/api/v1/status"};
char constexpr HTTP_API_V1_PLUGIN[] {"/api/v1/plugin/"};

char constexpr HTTP_HEADER[] {"<!DOCTYPE html><html><head><meta charset=\"utf-8\"><title>AM2H_Core</title><style>h1{color:blue;font-family:sans-serif;} body{color:grey;font-family: monospace;}</style></head><body>"};
char constexpr HTTP_FOOTER[] {"</body></html>\n"};

#define RETAINED true

#define JSON_CAPACITY 1500

// Config string length
constexpr uint8_t MQTT_SERVER_LEN {64};
constexpr uint8_t DEVICE_ID_LEN {16};
constexpr uint8_t NS_LEN {8};
constexpr uint8_t SSID_LEN {32};
constexpr uint8_t PW_LEN {32};
constexpr uint8_t SERVICE_LEN {8};
constexpr uint8_t LOC_LEN {32};

// Properties

String const DEVICE_CFG_TOPIC {"core"};
String const DEVICE_PROP_NAME {"config"};
String const STORAGE_PROP_NAME {"storage"};

#define DEVICE_ID_PROP_VAL "deviceId"
#define WLAN_PROP_NAME "wlan"
#define SSID_PROP_VAL "ssid"
#define PW_PROP_VAL "pw"
#define MQTT_PROP_NAME "mqtt"
#define MQTT_SERVER_PROP_VAL "mqttServer"
#define MQTT_PORT_PROP_VAL "mqttPort"
#define NS_PROP_VAL "ns"
#define CFG_PROP_NAME "cfg"
#define LOC_PROP_VAL "loc"
#define SERVICE_PROP_VAL "srv"
#define DATA_PROP_NAME "data"
String const STATUS_PROP_NAME {"status"};
String const RESET_PROP_NAME {"resetCause"};
String const HEAP_PROP_NAME {"freeHeap"};
String const RUN_PROP_NAME {"runningSince"};
String const VERSION_PROP_NAME {"fwVersion"};
String const IPADDRESS_PROP_NAME {"ipAddress"};
#define OFFLINE_PROP_VAL "offline"
#define CONFIG_PROP_VAL  "config"
#define ONLINE_PROP_VAL  "running"
#define MESSAGE_PROP_NAME "message"
#define ERROR_PROP_NAME "error"
String const ERROR_CODE {"errorCode"};
#endif
