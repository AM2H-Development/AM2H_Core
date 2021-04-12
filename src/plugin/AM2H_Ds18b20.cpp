#include "AM2H_Ds18b20.h"
#include "AM2H_Core.h"


// DS18B20: 28FFD059051603CC
// DS18B20: 28FFB09181150351
// DS18B20: 28FFC34001160448

void AM2H_Ds18b20::timerPublish(AM2H_Datastore& d, PubSubClient& mqttClient, const String topic){
    AM2H_Core::debugMessage("Publish: ");
    AM2H_Core::debugMessage(topic);
    mqttClient.publish((topic + "test").c_str() ,"payload");
}

void AM2H_Ds18b20::config(AM2H_Datastore& d, const MqttTopic& t, const String p){
    constexpr int setupComplete=7;
    AM2H_Core::debugMessage("Config:");
    AM2H_Core::debugMessage(p);
    AM2H_Core::debugMessage("\n");
    if (t.meas_ == "addr") {
        d.sensor.ds18b20.addr=p.toInt();
        d.config |=1;
    }
    if (t.meas_ == "loc") {
        for (int i=0; i < p.length(); ++i){
            d.loc[i]= p.charAt(i);
        }
        d.loc[p.length()]='\0';
        d.config |=2;
    }
    if (t.meas_ == "offsetTemp") {
        d.sensor.ds18b20.offsetTemp=p.toInt();
        d.config |=4;
    }
    if ( d.config == setupComplete ){
        // d.active = true;
        d.plugin = plugin_;
        d.self = this;
        AM2H_Core::debugMessage("Config finished\n");
    } else {
        d.self=nullptr;
        // d.active = false;
    }
}