#include "AM2H_Icounter.h"
#include "AM2H_Core.h"

void AM2H_Icounter::setupPlugin(){
    AM2H_Core::debugMessage("\nICounter::Setup\n");
}

void AM2H_Icounter::timerPublish(AM2H_Datastore& d, PubSubClient& mqttClient, const String topic){
    const uint32_t now = millis();
    const uint32_t interval = now - d.sensor.icounter.millis;

    AM2H_Core::debugMessage("Timer Publish: ");
    AM2H_Core::debugMessage(topic+"\n");
    
    mqttClient.publish((topic + "counter").c_str() ,String( calculateCounter(d) ).c_str());
    mqttClient.publish((topic + "power").c_str() ,String( calculatePower(d, interval) ).c_str());
}

void AM2H_Icounter::interruptPublish(AM2H_Datastore& d, PubSubClient& mqttClient, const String topic){
    ++d.sensor.icounter.absCnt;
    const uint32_t now = millis();
    const uint32_t interval = now - d.sensor.icounter.millis;
    d.sensor.icounter.millis = now;

    AM2H_Core::debugMessage("Int Publish: ");
    AM2H_Core::debugMessage(topic+"\n");
    mqttClient.publish((topic + "counter").c_str() ,String( calculateCounter(d) ).c_str());
    mqttClient.publish((topic + "power").c_str() ,String( calculatePower(d, interval) ).c_str());
}

double AM2H_Icounter::calculateCounter(AM2H_Datastore& d){
    return d.sensor.icounter.absCnt * d.sensor.icounter.unitsPerTick * pow (10., d.sensor.icounter.exponentPerTick);
}

double AM2H_Icounter::calculatePower(AM2H_Datastore& d, const uint32_t interval){
    AM2H_Core::debugMessage("Interval: " + String(interval) );
    if ( interval > d.sensor.icounter.zeroLimit ){
        return 0.;
    }
    const double d_counter = static_cast<double>(d.sensor.icounter.unitsPerTick) * pow (10., d.sensor.icounter.exponentPerTick); // counter_units
    const double d_interval = static_cast<double>(interval) * static_cast<double>(d.sensor.icounter.unitsPerMs) * pow(10., static_cast<double>(d.sensor.icounter.exponentPerMs)); // interval_units
    AM2H_Core::debugMessage(" d_counter: " + String(d_counter) );
    AM2H_Core::debugMessage(" d_interval: " + String(d_interval) + "\n" );    
    return  d_counter / d_interval;
}

void AM2H_Icounter::config(AM2H_Datastore& d, const MqttTopic& t, const String p){
    AM2H_Core::debugMessage("Config:");
    AM2H_Core::debugMessage(p);
    AM2H_Core::debugMessage("\n");
    if (t.meas_ == "absCnt") {
        d.sensor.icounter.absCnt=p.toInt();
        d.sensor.icounter.millis=0;
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