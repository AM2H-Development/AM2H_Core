#include "AM2H_Icounter.h"
#include "AM2H_Core.h"

void AM2H_Icounter::timerPublish(AM2H_Datastore& d, PubSubClient& mqttClient, const String topic){
    AM2H_Core::debugMessage("Timer Publish: ");
    AM2H_Core::debugMessage(topic);
    mqttClient.publish((topic + "test").c_str() ,"payload");
}

void AM2H_Icounter::interruptPublish(AM2H_Datastore& d, PubSubClient& mqttClient, const String topic){
    AM2H_Core::debugMessage("Int Publish: ");
    AM2H_Core::debugMessage(topic);
    mqttClient.publish((topic + "test").c_str() ,"payload");
}

void AM2H_Icounter::config(AM2H_Datastore& d, const MqttTopic& t, const String p){
    constexpr int setupComplete=127;
    AM2H_Core::debugMessage("Config:");
    AM2H_Core::debugMessage(p);
    AM2H_Core::debugMessage("\n");
    if (t.meas_ == "absCnt") {
        d.sensor.icounter.absCnt=p.toInt();
        d.config |=1;
    }
    if (t.meas_ == "unitsPerTick") {
        d.sensor.icounter.unitsPerTick=p.toInt();
        d.config |=2;
    }
    if (t.meas_ == "exponentPerTick") {
        d.sensor.icounter.exponentPerTick=p.toInt();
        d.config |=4;
    }
    if (t.meas_ == "unitsPerMs") {
        d.sensor.icounter.unitsPerMs=p.toInt();
        d.config |=8;
    }
    if (t.meas_ == "exponentPerMs") {
        d.sensor.icounter.exponentPerMs=p.toInt();
        d.config |=16;
    }
    if (t.meas_ == "zeroLimit") {
        d.sensor.icounter.zeroLimit=p.toInt();
        d.config |=32;
    }
    if (t.meas_ == "loc") {
        for (int i=0; i < p.length(); ++i){
            d.loc[i]= p.charAt(i);
        }
        d.loc[p.length()]='\0';
        d.config |=64;
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