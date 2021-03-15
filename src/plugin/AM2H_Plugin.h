#ifndef AM2H_Plugin_h
#define AM2H_Plugin_h

#include "Arduino.h"
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP_EEPROM.h>
#include "include/AM2H_Core_Constants.h"
#include "include/AM2H_Datastore.h"

class AM2H_Plugin{
  public:
    AM2H_Plugin(const char* plugin): plugin_(plugin){}
    // void activate() { active_ = true; }
    // inline const AM2H_Core& getCore(){if (core_) return *core_;}

    // void call_loop(){ if ( active_ ) loop(); }
    virtual void loop()=0;

    /*void call_setup(AM2H_Core& core){
      if ( active_ && !setupDone_ ) { 
        core_ = &core;
        setup();
        setupDone_=true;
      }
    }*/
    virtual void setup()=0;

    virtual void config(AM2H_Datastore& d, const MqttTopic t, const String payload)=0;

    /*void call_config(AM2H_Core& core, const char* key, const char* val){ 
      config( core, key, val );
    }
    virtual void config(AM2H_Core& core, const char* key, const char* val)=0;
    */
    String getSrv(){
      return srv_;
    }

  protected:
    String plugin_;

    // AM2H_Core* core_ = nullptr;
    bool isActive() { return active_;}
    String srv_ {"none"};

    bool active_ = false;
    // bool interrupt_ = false;
    bool setupDone_ = false;
};


#endif