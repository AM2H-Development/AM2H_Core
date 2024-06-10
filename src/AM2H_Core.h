#ifndef AM2H_Core_h
#define AM2H_Core_h
#include <Arduino.h>
#include <AM2H_Datastore.h>
#include <AM2H_Helper.h>
#include <Wire.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP_EEPROM.h>
#include <AM2H_Core_Constants.h>
#include <AM2H_MqttTopic.h>
#include "plugin/AM2H_Plugin.h"
#include <OneWire.h>
#include "bsec.h"

#define VERSION "1.8.3"

void IRAM_ATTR impulseISR();

struct PersistentSetupContainer {
  char mqttServer[MQTT_SERVER_LEN];
  int  mqttPort;
  char deviceId[DEVICE_ID_LEN];
  char ns[NS_LEN];
};

struct VolatileSetupContainer {
  String ssid;                // WLAN SSID
  String pw;                  // WLAN password
  uint8_t sampleRate;         // sample rate in seconds
  uint8_t i2cMuxAddr;         // multiplexer address
  String nickname;            // nickname for device
};

struct Timers {
  uint32_t wlanReconnect{0}; // non-blocking timer for Wlan reconnect
  uint32_t mqttReconnect{0}; // non-blocking timer for Mqtt reconnect
  uint32_t espRestart{0};    // non-blocking timer for ESP.restart();
  uint32_t sendData{0};      // non-blocking timer for sending data by Mqtt;
};

struct LogEntry {
  bool error{false};
  uint32_t ts{0};
  char caller[LOG_CALLER_LENGTH]="-";
  char message[LOG_MESSAGE_LENGTH]="-";
  uint16_t freeHeap{0};
};

class AM2H_Core {
public:
  explicit AM2H_Core(AM2H_Plugin** plugins);
  AM2H_Core(AM2H_Plugin** plugins, PubSubClient& mqttClient, ESP8266WebServer& server);

  void setupCore();
  void loopCore();
  void wait(uint32_t const del_millis);
  static MqttTopic parseMqttTopic(const char* topic);
  static void const debugMessage(const String& caller, const String& message, const bool newline, const bool info);
  static void const debugMessage(const String& caller, const String& message, const bool info){debugMessage(caller, message, false, info);};
  static void const debugMessageNl(const String& caller, const String& message, const bool info){debugMessage(caller, message, true, info);};
  static bool const parse_debugMessage (const String message, String& newMessage);
  void loopMqtt();
  void switchWire(uint32_t const addr) const;
  // void i2cReset();

private:
  AM2H_Plugin** plugins_;
  PubSubClient& mqttClient_;
public:
  ESP8266WebServer& server_;
private:
  LogEntry logbook[LOG_LENGTH];
  uint8_t logpos{0};
  uint32_t thershold{LOG_INFO_THRESHOLD};
  byte updateRequired_;
  byte connStatus_;
  PersistentSetupContainer persistentSetupValues_;
  VolatileSetupContainer volatileSetupValues_;
  Timers timer_;
  uint16_t mqttReconnectCounter;

  void setupEEPROM();
  bool writeEEPROM();
  void setupWlan();
  void setupWire();
  void restartWlan(String ssid, String pw);
  void connectWlan(int timeout);
  void setupServer();
  void publishDeviceStatus();
  void publishConfigDeviceStatus();
  void loopServer();
  void checkUpdateRequired();
  void checkTimerPublish();
  void checkIntPublish();
  void loopPlugins();
  static void handleRoot();
  static const String getRootContent();
  static void handleRestart();
  static void handleStatus();
  static void handleNotFound();
  static void handleApiGetRequest();
  static void handleApiSetRequest();
  static void handlePlugin(const int id);
  void setupMqtt();
  static void mqttCallback(const char* topic, uint8_t* payload, unsigned int length);
  void mqttReconnect();
  void scan() const;

public:
  // Getters/Setters:
  static const String getStatusTopic();
  static const String getConfigTopic();
  static const String getStorageTopic();
  static const String getFullStorageTopic(const String id, const String srv, const String meas);
  static const String getDataTopic(const String loc, const String srv, const String id);

  PubSubClient& getMqttClient() const { return mqttClient_; }
  const String getDeviceId() const { return persistentSetupValues_.deviceId; }
  const String getSSID() const { return volatileSetupValues_.ssid; }
  const String getPW() const { return volatileSetupValues_.pw; }
  const char* getMQTTServer() const { return persistentSetupValues_.mqttServer; }
  const int getMQTTPort() const { return persistentSetupValues_.mqttPort; }
  const String getNamespace() const { return persistentSetupValues_.ns; }
  const String getNickname() const { return volatileSetupValues_.nickname; }

  void subscribe(const char* topic);
  void unsubscribe(const char* topic);

  void setDeviceId(const String deviceId){
    deviceId.toCharArray(persistentSetupValues_.deviceId, DEVICE_ID_LEN);
    updateRequired_ |= COMMIT_TO_EEPROM_REQUIRED | MQTT_UPDATE_REQUIRED | MQTT_RESET_REQUIRED;
  }

  void setSSID(const String ssid){
    volatileSetupValues_.ssid = ssid;
    updateRequired_ |= WLAN_RESET_REQUIRED;
  }

  void setPW(const String pw){
    volatileSetupValues_.pw = pw;
    updateRequired_ |= WLAN_RESET_REQUIRED;
  }

  void setMQTTServer(const String mqttServer){
    mqttServer.toCharArray(persistentSetupValues_.mqttServer, MQTT_SERVER_LEN);
    updateRequired_ |= COMMIT_TO_EEPROM_REQUIRED | MQTT_RESET_REQUIRED;
  }

  void setMQTTPort(int mqttPort){
    persistentSetupValues_.mqttPort = mqttPort;
    updateRequired_ |= COMMIT_TO_EEPROM_REQUIRED | MQTT_RESET_REQUIRED;
  }

  void setMQTTPort(const String mqttPort){
    setMQTTPort(mqttPort.toInt());
  }

  void setNamespace(const String ns){
    ns.toCharArray(persistentSetupValues_.ns, NS_LEN);
    updateRequired_ |= COMMIT_TO_EEPROM_REQUIRED | MQTT_UPDATE_REQUIRED | MQTT_RESET_REQUIRED;
  }

  const bool isIntAvailable() const;
  const uint32_t getLastImpulseMillis() const;
};

#endif
