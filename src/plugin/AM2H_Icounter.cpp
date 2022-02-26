#include "AM2H_Icounter.h"
#include "AM2H_Core.h"
#include "include/AM2H_Datastore.h"
#include "include/AM2H_Helper.h"

extern unsigned long lastImpulseMillis_G;
extern AM2H_Core* am2h_core;

void AM2H_Icounter::timerPublish(AM2H_Datastore& d, PubSubClient& mqttClient, const String topic){
    AM2H_Core::debugMessage("AM2H_Icounter::timerPublish()","publishing to " + topic);

    const unsigned long timeout = (static_cast<unsigned long>(d.sensor.icounter.zeroLimit)*1000)+lastImpulseMillis_G;
    if ( timeout < millis() ){
        mqttClient.publish((topic + "power").c_str() ,"0");
        AM2H_Core::debugMessage("AM2H_Icounter::timerPublish()","done...");
    }
}

void AM2H_Icounter::interruptPublish(AM2H_Datastore& d, PubSubClient& mqttClient, const String topic){
    const uint32_t del = 30 - (millis() - lastImpulseMillis_G);
    AM2H_Core::debugMessage("AM2H_Icounter::interruptPublish()","Delay: "+ String(del) + " state:"+String(digitalRead(CORE_ISR_PIN)));
    if (del<30) {
        delay(del);
    }
    if (digitalRead(CORE_ISR_PIN)==0){
        AM2H_Core::debugMessage("AM2H_Icounter::interruptPublish()","skipped...");
        return;
    }

    ++d.sensor.icounter.absCnt;
    const uint32_t interval = lastImpulseMillis_G - d.sensor.icounter.millis;
    d.sensor.icounter.millis = lastImpulseMillis_G;

    AM2H_Core::debugMessage("AM2H_Icounter::interruptPublish()","interval is " + String(interval) + " publishing to " + topic);
    mqttClient.publish((topic + "counter").c_str() ,String( calculateCounter(d) ).c_str());
    am2h_core->loopMqtt();
    mqttClient.publish((topic + "power").c_str() ,String( calculatePower(d, interval) ).c_str());
    am2h_core->loopMqtt();
    mqttClient.publish(d.sensor.icounter.absCntTopic , String( d.sensor.icounter.absCnt ).c_str(), RETAINED);
    am2h_core->loopMqtt();
}

double AM2H_Icounter::calculateCounter(AM2H_Datastore& d){
    return d.sensor.icounter.absCnt * d.sensor.icounter.unitsPerTick * pow (10., d.sensor.icounter.exponentPerTick);
}

double AM2H_Icounter::calculatePower(AM2H_Datastore& d, const uint32_t interval){
    AM2H_Core::debugMessage("AM2H_Icounter::calculatePower()","Interval: " + String(interval) );
    const unsigned long timeout = (static_cast<unsigned long>(d.sensor.icounter.zeroLimit)*1000);
    if ( interval > timeout ){
        return 0.;
    }
    const double power = 1. / (static_cast<double>(interval)/1000) * static_cast<double>(d.sensor.icounter.unitsPerMs) * pow(10., static_cast<double>(d.sensor.icounter.exponentPerMs));
    AM2H_Core::debugMessage("AM2H_Icounter::calculatePower()"," power: " + String(power) );
    return  power;
}

void AM2H_Icounter::config(AM2H_Datastore& d, const MqttTopic& t, const String p){
    AM2H_Core::debugMessage("AM2H_Icounter::config()","old config state {"+String(d.config,BIN)+"}\n"+t.meas_+" : "+p);

    if (!d.initialized) {
        d.initialized=true;

        String tempTopic = am2h_core->getFullStorageTopic(String(t.id_), t.srv_, "absCnt");
        d.sensor.icounter.absCntTopic = new char[tempTopic.length()];
        size_t i=0;
        for (auto c: tempTopic ){
            d.sensor.icounter.absCntTopic[i++]=c;
        }
        d.sensor.icounter.absCntTopic[i]='\0';
        
        am2h_core->subscribe(d.sensor.icounter.absCntTopic);
    }

    if (t.meas_ == "absCnt") {
        d.sensor.icounter.absCnt=p.toInt();
        d.sensor.icounter.millis=(UINT32_MAX/2);
        am2h_core->unsubscribe(d.sensor.icounter.absCntTopic);
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
        d.config &= ~Config::SET_6;
            if (p.length()>0) {
            AM2H_Helper::parse_location(d.loc,p);
            d.config |= Config::SET_6;
        }
    }
    if ( d.config == Config::CHECK_TO_6 ){
        // d.plugin = plugin_;
        d.self = this;
        pinMode(CORE_ISR_PIN, INPUT_PULLUP);
        AM2H_Core::debugMessage("AM2H_Icounter::config()","finished, new config state {"+String(d.config,BIN)+"}");
    } else {
        d.self=nullptr;
        // d.active = false;
    }
}