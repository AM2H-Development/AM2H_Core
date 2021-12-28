#include "AM2H_Bh1750.h"
#include "AM2H_Core.h"
#include "libs/BH1750/BH1750.h"
#include <Wire.h>

extern AM2H_Core* am2h_core;

void AM2H_Bh1750::timerPublish(AM2H_Datastore& d, PubSubClient& mqttClient, const String topic){
    float level = -5.0;
    if (lightMeter.measurementReady()) {
        level = lightMeter.readLightLevel();
    }
    AM2H_Core::debugMessage("AM2H_Bh1750::timerPublish()","publishing to " + topic + "illuminance="+String(level));
    mqttClient.publish( (topic + "illuminance").c_str() , String( level ).c_str() );
}

void AM2H_Bh1750::config(AM2H_Datastore& d, const MqttTopic& t, const String p){
    AM2H_Core::debugMessage("AM2H_Bh1750::config()","old config state {"+String(d.config,BIN)+"}\n");

    if (t.meas_ == "addr") {
        d.sensor.bh1750.addr= AM2H_Helper::parse_hex<uint32_t>(p);
        AM2H_Core::debugMessage("AM2H_Bh1750::config()","set addr = 0x"+String(d.sensor.bh1750.addr,HEX)+"\n");
        d.config |= Config::SET_0;
    }
    if (t.meas_ == "loc") {
        AM2H_Helper::parse_location(d.loc,p);
        AM2H_Core::debugMessage("AM2H_Bh1750::config()","set loc = "+String(d.loc)+"\n");
        d.config |= Config::SET_1;
    }
    if (t.meas_ == "sensitivityAdjust") {
        d.sensor.bh1750.sensitivityAdjust=p.toInt();
        if (d.sensor.bh1750.sensitivityAdjust<32) {d.sensor.bh1750.sensitivityAdjust=32;}
        AM2H_Core::debugMessage("AM2H_Bh1750::config()","set sensitivityAdjust = "+String(d.sensor.bh1750.sensitivityAdjust)+"\n");
        d.config |= Config::SET_2;
    }
    if ( d.config == Config::CHECK_TO_2 ){
        // d.plugin = plugin_;
        d.self = this;
        postConfig(d);
        AM2H_Core::debugMessage("AM2H_Bh1750::config()","finished, new config state {"+String(d.config,BIN)+"}\n");
    } else {
        d.self=nullptr;
    }
}

void AM2H_Bh1750::postConfig(AM2H_Datastore& d){
    const uint8_t addr = static_cast<uint8_t>(d.sensor.bh1750.addr);
    if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE,addr)) {
       AM2H_Core::debugMessage("AM2H_Bh1750::postConfig()","BH1750 initialized. Setting sensitivity to " + d.sensor.bh1750.sensitivityAdjust);
    } else {
        AM2H_Core::debugMessage("AM2H_Bh1750::postConfig()","Error initialising BH1750");
    }
    lightMeter.setMTreg(d.sensor.bh1750.sensitivityAdjust);
}