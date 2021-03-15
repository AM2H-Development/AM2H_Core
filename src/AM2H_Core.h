#ifndef AM2H_Core_h
#define AM2H_Core_h

#include "Arduino.h"
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP_EEPROM.h>
#include "include/AM2H_Core_Constants.h"
#include "include/AM2H_MqttTopic.h"
#include "AM2H_Plugin.h"

void ICACHE_RAM_ATTR impulseISR();

struct PersistentSetupContainer {
  char mqttServer[MQTT_SERVER_LEN];
  int  mqttPort;
  char deviceId[DEVICE_ID_LEN];
  char ns[NS_LEN];
};

struct VolatileSetupContainer {
  String ssid;                // WLAN SSID
  String pw;                  // WLAN password
  int sampleRate;             // Sample rate in seconds
};

struct Timers {
  unsigned long wlanReconnect; // non-blocking timer for Wlan reconnect
  unsigned long mqttReconnect; // non-blocking timer for Mqtt reconnect
  unsigned long espRestart;    // non-blocking timer for ESP.restart();
  unsigned long sendData;      // non-blocking timer for sending data by Mqtt;
};

class AM2H_Core {
public:
  AM2H_Core(AM2H_Plugin** plugins, PubSubClient& mqttClient, ESP8266WebServer& server);
  void setup();
  void loop();
  static void debugMessage(String message);
  static MqttTopic parseMqttTopic(char* topic);
    
private:
  AM2H_Plugin** plugins_;

  PubSubClient& mqttClient_;
  ESP8266WebServer& server_;
  String status_;
  byte updateRequired_;
  byte connStatus_;

  PersistentSetupContainer persistentSetupValues_;
  VolatileSetupContainer volatileSetupValues_;

  Timers timer_;

  void setupEEPROM();
  void writeEEPROM();
  void setupWlan(); 
  void restartWlan(String ssid, String pw);
  void connectWlan(int timeout);
  void setupServer();
  void loopServer();
  void checkUpdateRequired();
  static void handleRoot();
  static void handleRestart();
  static void handleStatus();
  static void handleNotFound();
  static void handleApiRequest();
  static void handleApiGetRequest();
  static void handleApiSetRequest();

  void setupMqtt();
  void loopMqtt();
  static void mqttCallback(char* topic, uint8_t* payload, unsigned int length);
  void mqttReconnect();

public:
  // Getters/Setters:
  static String getStatusTopic();
  static String getConfigTopic();

  PubSubClient getMqttClient(){
    return mqttClient_;
  }
  
  String getDeviceId(){
    return persistentSetupValues_.deviceId;
  }

  void setDeviceId(const String deviceId){
    deviceId.toCharArray(persistentSetupValues_.deviceId, DEVICE_ID_LEN);
    updateRequired_ |= COMMIT_TO_EEPROM_REQUIRED | MQTT_UPDATE_REQUIRED | MQTT_RESET_REQUIRED;    
  }

  String getSSID(){
    return volatileSetupValues_.ssid;
  }

  void setSSID(const String ssid){
    volatileSetupValues_.ssid = ssid;
    updateRequired_ |= WLAN_RESET_REQUIRED;
  }

  String getPW(){
    return volatileSetupValues_.pw;
  }

  void setPW(const String pw){
    volatileSetupValues_.pw = pw;
    updateRequired_ |= WLAN_RESET_REQUIRED;
  }

  String getMQTTServer(){
    return persistentSetupValues_.mqttServer;
  }

  void setMQTTServer(const String mqttServer){
    mqttServer.toCharArray(persistentSetupValues_.mqttServer, MQTT_SERVER_LEN);
    updateRequired_ |= COMMIT_TO_EEPROM_REQUIRED | MQTT_RESET_REQUIRED;
}

  int getMQTTPort(){
    return persistentSetupValues_.mqttPort;
  }

  void setMQTTPort(int mqttPort){
    persistentSetupValues_.mqttPort = mqttPort;
    updateRequired_ |= COMMIT_TO_EEPROM_REQUIRED | MQTT_RESET_REQUIRED;
  }

  void setMQTTPort(const String mqttPort){
    setMQTTPort(mqttPort.toInt());
  }

  String getNamespace(){
    return persistentSetupValues_.ns;
  }

  void setNamespace(const String ns){
    ns.toCharArray(persistentSetupValues_.ns, NS_LEN);
    updateRequired_ |= COMMIT_TO_EEPROM_REQUIRED | MQTT_UPDATE_REQUIRED | MQTT_RESET_REQUIRED;
  }

  bool isIntAvailable();
  void resetInt();
  unsigned long getLastImpulseMillis();
};

class AM2H_Plugin{
  public:
    void activate() { active_ = true; }
    inline const AM2H_Core& getCore(){if (core_) return *core_;}

    void call_loop(){ if ( active_ ) loop(); }
    virtual void loop()=0;

    void call_setup(AM2H_Core& core){
      if ( active_ && !setupDone_ ) { 
        core_ = &core;
        setup();
        setupDone_=true;
      }
    }
    virtual void setup()=0;

    void call_config(AM2H_Core& core, const char* key, const char* val){ 
      config( core, key, val );
    }
    virtual void config(AM2H_Core& core, const char* key, const char* val)=0;

  protected:
    AM2H_Core* core_ = nullptr;
    bool isActive() { return active_;}
    String srv_ {"none"};

  private:
    bool active_ = false;
    // bool interrupt_ = false;
    bool setupDone_ = false;
};


#endif
