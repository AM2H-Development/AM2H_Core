#ifndef AM2H_Core_Constants_h
#define AM2H_Core_Constants_h

// Pin Setup
byte constexpr CORE_SCL{0};
byte constexpr CORE_ISR_PIN{1};
byte constexpr CORE_STATUS_LED{2};
byte constexpr CORE_SDA{2};
byte constexpr CORE_DQ{4};


// Connection Status
byte constexpr DEV_RESTART_PENDING   {0x00};
byte constexpr CONN_UNKNOWN          {0x01};
byte constexpr WLAN_AP_PROV          {0x02};
byte constexpr WLAN_CONNECTED        {0x03};
byte constexpr MQTT_CLIENT_CONNECTED {0x04};
byte constexpr DEV_CONFIGURED        {0x05};

// API Updates
byte constexpr NO_UPDATE_REQUIRED        {0b00000001};
byte constexpr COMMIT_TO_EEPROM_REQUIRED {0b00000010};
byte constexpr WLAN_RESET_REQUIRED       {0b00000100};
byte constexpr MQTT_RESET_REQUIRED       {0b00001000};
byte constexpr MQTT_UPDATE_REQUIRED      {0b00010000};
// #define MQTT_PUBLISH_MESSAGE      0b00100000
byte constexpr ESP_RESET_REQUIRED        {0b10000000};

// MQTT Updates
int constexpr WLAN_TIMEOUT{20}; // Wlan timeout in s
// #define ENCODING_HTML "text/html; charset=UTF-8"

// HTTP Api
char constexpr ENCODING_PLAIN[] {"text/plain; charset=UTF-8"};
char constexpr ENCODING_JSON[] {"application/json; charset=utf-8"};
char constexpr HTTP_API_V1_SET[] {"/api/v1/set"};
char constexpr HTTP_API_V1_GET[] {"/api/v1/get"};
char constexpr HTTP_API_V1_RESTART[]  {"/api/v1/restart"};
char constexpr HTTP_API_V1_STATUS[] {"/api/v1/status"};

#define RETAINED true

#define JSON_CAPACITY 1500

// Config string length
byte constexpr MQTT_SERVER_LEN {64};
byte constexpr DEVICE_ID_LEN {16};
byte constexpr NS_LEN {8};
byte constexpr SSID_LEN {32};
byte constexpr PW_LEN {32};
byte constexpr SERVICE_LEN {8};
byte constexpr LOC_LEN {32};

// Properties
#define DEVICE_PROP_NAME "dev"
#define   DEVICE_ID_PROP_VAL "deviceId"
#define WLAN_PROP_NAME "wlan"
#define   SSID_PROP_VAL "ssid"
#define   PW_PROP_VAL "pw"
#define MQTT_PROP_NAME "mqtt"
#define   MQTT_SERVER_PROP_VAL "mqttServer"
#define   MQTT_PORT_PROP_VAL "mqttPort"
#define   NS_PROP_VAL "ns"
#define CFG_PROP_NAME "cfg"
#define   LOC_PROP_VAL "loc"
#define   SERVICE_PROP_VAL "srv"
#define DATA_PROP_NAME "data"
#define STATUS_PROP_NAME "status"
#define   OFFLINE_PROP_VAL "offline"
#define   CONFIG_PROP_VAL  "config"
#define   ONLINE_PROP_VAL  "online"
#define MESSAGE_PROP_NAME "message"
#define ERROR_PROP_NAME "error"

#endif
