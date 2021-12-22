#include "AM2H_Bh1750.h"
#include "AM2H_Core.h"
#include "libs/BH1750/BH1750.h"
#include <Wire.h>

extern AM2H_Core* am2h_core;

void AM2H_Bh1750::setupPlugin(int datastoreIndex){
    AM2H_Core::debugMessage("\nBh1750::Setup\n");
}

void AM2H_Bh1750::timerPublish(AM2H_Datastore& d, PubSubClient& mqttClient, const String topic){
    AM2H_Core::debugMessage("Publish: ");

    float level = -5.0;
    if (lightMeter.measurementReady()) {
        level = lightMeter.readLightLevel();
    }
    AM2H_Core::debugMessage(String(level));

    mqttClient.publish( (topic + "illuminance").c_str() , String( level ).c_str() );
}

void AM2H_Bh1750::config(AM2H_Datastore& d, const MqttTopic& t, const String p){
    AM2H_Core::debugMessage("Bh1750::Config (" + String(d.config,BIN) + "):");
    AM2H_Core::debugMessage(p);
    AM2H_Core::debugMessage("\n");
    if (t.meas_ == "addr") {
      AM2H_Core::debugMessage("Addr = 0x");
      d.sensor.bh1750.addr= AM2H_Helper::parse_hex<uint32_t>(p);
      AM2H_Core::debugMessage(String(d.sensor.bh1750.addr,HEX));
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
    if (t.meas_ == "sensitivityAdjust") {
        d.sensor.bh1750.sensitivityAdjust=p.toInt();
        if (d.sensor.bh1750.sensitivityAdjust<31) {d.sensor.bh1750.sensitivityAdjust=31;}
        d.config |= Config::SET_2;
    }
    if ( d.config == Config::CHECK_TO_2 ){
        d.plugin = plugin_;
        d.self = this;
        postConfig(d);
        AM2H_Core::debugMessage("Config finished\n");
    } else {
        d.self=nullptr;
    }
}

void AM2H_Bh1750::postConfig(AM2H_Datastore& d){
    const uint8_t addr = static_cast<uint8_t>(d.sensor.bh1750.addr);
    if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE,addr)) {
        AM2H_Core::debugMessage("BH1750 Advanced begin");
    } else {
        AM2H_Core::debugMessage("Error initialising BH1750");
    }
    lightMeter.setMTreg(d.sensor.bh1750.sensitivityAdjust);
}