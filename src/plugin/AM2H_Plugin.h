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

class AM2H_Plugin{
  public:
    AM2H_Plugin(const char* plugin, const char* srv): plugin_(plugin), srv_(srv){};
    virtual void loopPlugin(int datastoreIndex){};
    virtual void setupPlugin(int datastoreIndex){};
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

class AM2H_Helper {
  public:
    template <class T> const static T parse_hex(const String& string){
        T number{0};
        uint32_t shift{0};

        int len = string.length();
        for (int pos=0;  pos<len; ++pos){
            char c = string.charAt(len-pos-1);
            if ( c >= '0' && c <='9' ){ number += (c-'0') << shift; }
            if ( c >= 'A' && c <= 'F' ){ number+= (c-'A'+10) << shift;}
            if ( c >= 'a' && c <= 'f' ){ number+= (c-'a'+10) << shift;}
            if ( c=='x' ) {break;}
            shift+=4;
        }
        return number;
    }
};

#endif