#include "AM2H_Sht21.h"
#include "AM2H_Core.h"
#include "libs/SHT21/SHT21.h"

extern AM2H_Core* am2h_core;

void AM2H_Sht21::setupPlugin(int datastoreIndex){
  AM2H_Core::debugMessage("\nSht21::Setup\n");
  sht21.begin();
}

void AM2H_Sht21::timerPublish(AM2H_Datastore& d, PubSubClient& mqttClient, const String topic){
  AM2H_Core::debugMessage("Publish: ");

  float celsius = sht21.getTemperature();
  float humidity = sht21.getHumidity();
  celsius += ( (float) d.sensor.sht21.offsetTemp / 10.0);
  humidity += ( (float) d.sensor.sht21.offsetHumidity / 10.0);

  mqttClient.publish( (topic + "temperature").c_str() , String( celsius ).c_str() );
  mqttClient.publish( (topic + "humidity").c_str() , String( humidity ).c_str() );
}

void AM2H_Sht21::config(AM2H_Datastore& d, const MqttTopic& t, const String p){
    AM2H_Core::debugMessage("Sht21::Config (" + String(d.config,BIN) + "):");
    AM2H_Core::debugMessage(p);
    AM2H_Core::debugMessage("\n");
    if (t.meas_ == "addr") {
      AM2H_Core::debugMessage("Addr = ");
      d.sensor.sht21.addr=p.toInt();
      AM2H_Core::debugMessage(String(d.sensor.sht21.addr,HEX));
      AM2H_Core::debugMessage("\n");
      d.config |= Config::SET_0;
    }
    if (t.meas_ == "loc") {
        for (int i=0; i < p.length(); ++i){
            d.loc[i]= p.charAt(i);
        }
        d.loc[p.length()]='\0';
        d.config |= Config::SET_1;
    }
    if (t.meas_ == "offsetTemp") {
        d.sensor.sht21.offsetTemp=p.toInt();
        d.config |= Config::SET_2;
    }
    if (t.meas_ == "offsetHumidity") {
        d.sensor.sht21.offsetHumidity=p.toInt();
        d.config |= Config::SET_3;
    }
    if ( d.config == Config::CHECK_TO_3 ){
        d.plugin = plugin_;
        d.self = this;
        AM2H_Core::debugMessage("Config finished\n");
    } else {
        d.self=nullptr;
    }
}