#include "AM2H_Bh1750.h"
#include "AM2H_Core.h"
#include <Wire.h>

extern AM2H_Core* am2h_core;

void AM2H_Bh1750::setupPlugin(int datastoreIndex){
    AM2H_Core::debugMessage("\nBh1750::Setup\n");
}

void AM2H_Bh1750::timerPublish(AM2H_Datastore& d, PubSubClient& mqttClient, const String topic){
    AM2H_Core::debugMessage("Publish: ");

    float level = -1.0;
    const uint8_t addr = static_cast<uint8_t>(d.sensor.bh1750.addr);
    /*
        Wire.beginTransmission(addr);
        Wire.write(CONTINUOUS_HIGH_RES_MODE_2);
        Wire.endTransmission();

        delay(400);
*/
    Wire.requestFrom(addr, static_cast<uint8_t>(2));
      u_int16_t timeout{1000};

    while( (Wire.available() < 2) && (--timeout>0) ) {
        delay(1);
    }

    if ( timeout>0 ){
        unsigned int tmp = 0;
        tmp = Wire.read();
        AM2H_Core::debugMessage(String(tmp,BIN));
        tmp <<= 8;
        tmp |= Wire.read();
        AM2H_Core::debugMessage(" "+String(tmp,BIN));
        level = tmp;
    }

    if (level != -1.0) {
        level *= (69 / static_cast<float>(d.sensor.bh1750.sensitivityAdjust)) /2.4;
    }

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
        d.config |= Config::SET_2;
    }
    if ( d.config == Config::CHECK_TO_2 ){
        d.plugin = plugin_;
        d.self = this;

        const uint8_t addr = static_cast<uint8_t>(d.sensor.bh1750.addr);
        Wire.beginTransmission(addr);
        Wire.write((0b01000 << 3) | (d.sensor.bh1750.sensitivityAdjust >> 5));
        Wire.endTransmission();
        Wire.beginTransmission(addr);
        Wire.write((0b011 << 5 )  | (d.sensor.bh1750.sensitivityAdjust & 0b11111));
        Wire.endTransmission();
        Wire.beginTransmission(addr);
        Wire.write(CONTINUOUS_HIGH_RES_MODE_2);
        Wire.endTransmission();

        AM2H_Core::debugMessage("Config finished\n");
    } else {
        d.self=nullptr;
    }
}
