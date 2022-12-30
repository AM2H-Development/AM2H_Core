#ifndef AM2H_Plugin_h
#define AM2H_Plugin_h

#include "Arduino.h"
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <libs/PubSubClient/PubSubClient.h>
#include <libs/ESP_EEPROM/ESP_EEPROM.h>
#include "include/AM2H_Core_Constants.h"
#include "include/AM2H_Datastore.h"
#include "include/AM2H_MqttTopic.h"
// #include "include/AM2H_Helper.h"

class AM2H_Plugin{
  public:
    AM2H_Plugin(const char* plugin, const char* srv): plugin_(plugin), srv_(srv){};
    virtual void loopPlugin(AM2H_Datastore& d, const uint8_t index){};
    virtual void setupPlugin(){};
    virtual void config(AM2H_Datastore& d, const MqttTopic& t, const String payload)=0;
    virtual void postConfig(AM2H_Datastore& d){};
    virtual void timerPublish(AM2H_Datastore& d, PubSubClient& mqttClient, const String topic, const uint8_t index){};
    virtual void interruptPublish(AM2H_Datastore& d, PubSubClient& mqttClient, const String topic, const uint8_t index){};
    virtual String getHtmlTabName(){return plugin_;};
    virtual String getHtmlTabContent(){return "<p>" + plugin_ + "</p>";};

    const String getSrv() const { return srv_; }
    const String getPlugin() const { return plugin_; }

  protected:
    String plugin_{""}; // name of the plugin
    String srv_ {"none"};
};

#endif