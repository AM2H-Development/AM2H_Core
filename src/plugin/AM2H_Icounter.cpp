#include "AM2H_Icounter.h"
#include "AM2H_Core.h"

void AM2H_Icounter::timerPublish(AM2H_Datastore& d, PubSubClient& mqttClient, const String topic){
    AM2H_Core::debugMessage("Timer Publish: ");
    AM2H_Core::debugMessage(topic+"\n");
    mqttClient.publish((topic + "test").c_str() ,"payload");
}

void AM2H_Icounter::interruptPublish(AM2H_Datastore& d, PubSubClient& mqttClient, const String topic){
    AM2H_Core::debugMessage("Int Publish: ");
    AM2H_Core::debugMessage(topic+"\n");
    mqttClient.publish((topic + "test").c_str() ,"payload");
}

void AM2H_Icounter::config(AM2H_Datastore& d, const MqttTopic& t, const String p){
    AM2H_Core::debugMessage("Config:");
    AM2H_Core::debugMessage(p);
    AM2H_Core::debugMessage("\n");
    if (t.meas_ == "absCnt") {
        d.sensor.icounter.absCnt=p.toInt();
        d.config |= Config::SET_0;
    }
    if (t.meas_ == "unitsPerTick") {
        d.sensor.icounter.unitsPerTick=p.toInt();
        d.config |= Config::SET_1;
    }
    if (t.meas_ == "exponentPerTick") {
        d.sensor.icounter.exponentPerTick=p.toInt();
        d.config |= Config::SET_2;
    }
    if (t.meas_ == "unitsPerMs") {
        d.sensor.icounter.unitsPerMs=p.toInt();
        d.config |= Config::SET_3;
    }
    if (t.meas_ == "exponentPerMs") {
        d.sensor.icounter.exponentPerMs=p.toInt();
        d.config |= Config::SET_4;
    }
    if (t.meas_ == "zeroLimit") {
        d.sensor.icounter.zeroLimit=p.toInt();
        d.config |= Config::SET_5;
    }
    if (t.meas_ == "loc") {
        for (int i=0; i < p.length(); ++i){
            d.loc[i]= p.charAt(i);
        }
        d.loc[p.length()]='\0';
        d.config |= Config::SET_6;
    }
    AM2H_Core::debugMessage("set:");
    AM2H_Core::debugMessage(String(d.config,BIN));
    AM2H_Core::debugMessage("#\n");

    if ( d.config == Config::CHECK_TO_6 ){
        d.plugin = plugin_;
        d.self = this;
        AM2H_Core::debugMessage("Config finished\n");
    } else {
        d.self=nullptr;
        // d.active = false;
    }
}