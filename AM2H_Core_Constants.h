#ifndef AM2H_Core_Constants_h
#define AM2H_Core_Constants_h


// Connection Status
#define CONN_UNKNOWN            0x01
#define WLAN_AP_PROV            0x02
#define WLAN_CONNECTED          0x03
#define MQTT_CLIENT_CONNECTED   0x04
#define DEV_CONFIGURED          0x05
#define DEV_RESTART_PENDING     0x00

// API Updates
#define NO_UPDATE_REQUIRED        0b00000001
#define COMMIT_TO_EEPROM_REQUIRED 0b00000010
#define WLAN_RESET_REQUIRED       0b00000100
#define MQTT_RESET_REQUIRED       0b00001000
#define MQTT_UPDATE_REQUIRED      0b00010000
#define MQTT_PUBLISH_MESSAGE      0b00100000
#define ESP_RESET_REQUIRED        0b10000000

// MQTT Updates
#define WLAN_TIMEOUT 20 // Wlan timeout in s
#define IMPULSE_ISR_PIN 2 // Interrupt on GPIO2 / SDA
#define ENCODING_HTML "text/html; charset=UTF-8"
#define ENCODING_PLAIN "text/plain; charset=UTF-8"
#define ENCODING_JSON "application/json; charset=utf-8"
#define API_VERSION "/api/v1/"
#define API_RESTART API_VERSION "restart"
#define API_STATUS API_VERSION "status"
#define RETAINED true

#define JSON_CAPACITY 1500

// Config string length
#define MQTT_SERVER_LEN 64
#define DEVICE_ID_LEN 16
#define NS_LEN 8
#define SSID_LEN 32
#define PW_LEN 32
#define SERVICE_LEN 8
#define LOC_LEN 32

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