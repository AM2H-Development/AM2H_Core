#ifndef AM2H_Plugin_h
#define AM2H_Plugin_h

#include "Arduino.h"
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP_EEPROM.h>
#include "include/AM2H_Core_Constants.h"
#include "include/AM2H_Datastore.h"
#include "include/AM2H_MqttTopic.h"

class AM2H_Plugin{
  public:
    AM2H_Plugin(const char* plugin, const char* srv): plugin_(plugin), srv_(srv){}
    virtual void loop(int i){};
    virtual void setup(){};
    virtual void config(AM2H_Datastore& d, const MqttTopic& t, const String payload)=0;
    virtual void timerPublish(AM2H_Datastore& d, PubSubClient& mqttClient, const String topic){};
    virtual void interruptPublish(AM2H_Datastore& d, PubSubClient& mqttClient, const String topic){};

    String getSrv(){
      return srv_;
    }

    String getPlugin(){
      return plugin_;
    }

  protected:
    String plugin_{""};
    String srv_ {"none"};

    // bool isActive() { return active_;}
    // bool active_ = false;
    // bool interrupt_ = false;
    // bool setupDone_ = false;
};


#endif