#ifndef AM2H_Core_h
#define AM2H_Core_h

#include "Arduino.h"
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP_EEPROM.h>
#include <ArduinoJson.h>
#include "AM2H_Core_Constants.h"

class Plugin;

void ICACHE_RAM_ATTR impulseISR();

struct PersistentSetupContainer {
  char mqttServer[MQTT_SERVER_LEN];
  int mqttPort;
  char deviceId[DEVICE_ID_LEN];
  char ns[NS_LEN];
};

struct VolatileSetupContainer {
  // PersistentSetupContainer* p;
  String ssid;
  String pw;
  // char  srv[SERVICE_LEN];  /// ???????????????? Todo -> Library
  // char  loc[LOC_LEN]; /// ???????????????? Todo -> Library
};

struct Timers {
  unsigned long wlanReconnect; // non-blocking timer for Wlan reconnect
  unsigned long mqttReconnect; // non-blocking timer for Mqtt reconnect
  unsigned long espRestart;    // non-blocking timer for ESP.restart();
};

class AM2H_Core
{
  public:
    AM2H_Core(Plugin** plugins, PubSubClient& mqttClient, ESP8266WebServer& server);
    void setup();
    void loop();
    static void impulseISR();
    static void debugMessage(String message);
    
  private:
    Plugin** plugins_;

    unsigned long lastImpulseMillis_;
    unsigned long impulsesTotal_;
    bool intAvailable_;
    // unsigned long timespan; // ???
    // bool dataSent = true; // ???

    using ServerHandler = void (*)();
    ServerHandler handleRootH;

    using MqttHandler = void (*)(char*, uint8_t*, unsigned int);
    MqttHandler mqttCallbackH;

    PubSubClient& mqttClient_;
    ESP8266WebServer& server_;
    String status_;
    byte updateRequired_;
    byte connStatus_;

  public:
    PersistentSetupContainer persistentSetupValues_;
  private:
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
    void handleApiGetRequest(String& content);
    void handleApiPostRequest(String& content, const char* jsonPayload);

    void jsonifySetupValues(String& jsonString);

    void setupMqtt();
    void loopMqtt();
    void mqttCallback(char* topic, uint8_t* payload, unsigned int length);
    void mqttReconnect();

    void mqttPublishApiConfig();

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
    String getSSID(){
      return volatileSetupValues_.ssid;
    }
    String getPW(){
      return volatileSetupValues_.pw;
    }
    String getMQTTServer(){
      return persistentSetupValues_.mqttServer;
    }
    int getMQTTPort(){
      return persistentSetupValues_.mqttPort;
    }
    void setMQTTPort(int mqttPort){
      persistentSetupValues_.mqttPort = mqttPort;
    }
    String getNamespace(){
      return persistentSetupValues_.ns;
    }
    /*
    static char* getLocation(){ /// ???????????????? Todo -> Library
      return volatileSetupValues_.loc;
    }
    static char* getService(){ /// ???????????????? Todo -> Library
      return volatileSetupValues_.srv;
    }
    */
    bool isIntAvailable(){
      return intAvailable_;
    }
    void resetInt(){
      intAvailable_=false;
    }
    unsigned long getLastImpulseMillis(){
      return lastImpulseMillis_;
    }
};

class Plugin{
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