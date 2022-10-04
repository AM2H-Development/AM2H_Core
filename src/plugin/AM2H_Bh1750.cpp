#include "AM2H_Bh1750.h"
#include "include/AM2H_Helper.h"
#include "AM2H_Core.h"
#include "libs/BH1750/BH1750.h"
#include <Wire.h>

extern AM2H_Core* am2h_core;

void AM2H_Bh1750::timerPublish(AM2H_Datastore& d, PubSubClient& mqttClient, const String topic, const uint8_t index){
    float level = -5.0;
    if (lightMeter.measurementReady()) {
        am2h_core->switchWire(d.sensor.bh1750.addr);
        level = lightMeter.readLightLevel();
    }
    AM2H_Core::debugMessageNl("AM2H_Bh1750::timerPublish()","publishing to " + topic + "illuminance="+String(level), DebugLogger::INFO);
    char error[] = "1";
    if ( level > -5. ) {
        error[0] = '0';
        mqttClient.publish( (topic + "illuminance").c_str() , String( level ).c_str() );
        am2h_core->loopMqtt();
    } else {
        AM2H_Core::debugMessage("AM2H_Bh1750::timerPublish()","Error " + String(error), DebugLogger::ERROR);
        am2h_core->i2cReset();
    }
    
    mqttClient.publish( (topic + ERROR_CODE + "_" + String(index)).c_str(), error );
    am2h_core->loopMqtt();
}

void AM2H_Bh1750::config(AM2H_Datastore& d, const MqttTopic& t, const String p){
    AM2H_Core::debugMessageNl("AM2H_Bh1750::config()","old config state {"+String(d.config,BIN)+"}", DebugLogger::INFO);

    if (t.meas_ == "addr") {
        d.sensor.bh1750.addr= AM2H_Helper::parse_hex<uint32_t>(p);
        AM2H_Core::debugMessage("AM2H_Bh1750::config()"," set addr = 0x"+String(d.sensor.bh1750.addr,HEX), DebugLogger::INFO);
        d.config |= Config::SET_0;
    }
    if (t.meas_ == "loc") {
        d.config &= ~Config::SET_1;
            if (p.length()>0) {
            AM2H_Helper::parse_location(d.loc,p);
            AM2H_Core::debugMessage("AM2H_Bh1750::config()"," set loc = "+String(d.loc), DebugLogger::INFO);
            d.config |= Config::SET_1;
        }
    }
    if (t.meas_ == "sensitivityAdjust") {
        d.sensor.bh1750.sensitivityAdjust=p.toInt();
        if (d.sensor.bh1750.sensitivityAdjust<32 || d.sensor.bh1750.sensitivityAdjust>254 ) {d.sensor.bh1750.sensitivityAdjust=69;}
        AM2H_Core::debugMessage("AM2H_Bh1750::config()"," set sensitivityAdjust = "+String(d.sensor.bh1750.sensitivityAdjust), DebugLogger::INFO);
        d.config |= Config::SET_2;
    }
    if ( d.config == Config::CHECK_TO_2 ){
        // d.plugin = plugin_;
        d.self = this;
        postConfig(d);
        AM2H_Core::debugMessageNl("AM2H_Bh1750::config()","finished, new config state {"+String(d.config,BIN)+"}", DebugLogger::INFO);
    } else {
        d.self=nullptr;
    }
}

void AM2H_Bh1750::postConfig(AM2H_Datastore& d){
    const uint8_t addr = static_cast<uint8_t>(d.sensor.bh1750.addr);
    am2h_core->switchWire(d.sensor.bh1750.addr);
    if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE,addr)) {
       AM2H_Core::debugMessage("AM2H_Bh1750::postConfig()","BH1750 initialized. Setting sensitivity to " + String(d.sensor.bh1750.sensitivityAdjust), DebugLogger::INFO);
    } else {
        AM2H_Core::debugMessage("AM2H_Bh1750::postConfig()","Error initialising BH1750", DebugLogger::ERROR);
    }
    lightMeter.setMTreg(d.sensor.bh1750.sensitivityAdjust);
}