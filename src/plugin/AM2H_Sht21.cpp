#include "AM2H_Sht21.h"
#include "AM2H_Core.h"
#include "libs/SHT21/SHT21.h"

extern AM2H_Core* am2h_core;

void AM2H_Sht21::setupPlugin(){
  AM2H_Core::debugMessage("AM2H_Sht21::setupPlugin()","scanning for devices: ");
  scan();
}

void AM2H_Sht21::timerPublish(AM2H_Datastore& d, PubSubClient& mqttClient, const String topic){
  AM2H_Core::debugMessage("AM2H_Sht21::timerPublish()","publishing ...");

  float celsius = sht21.getTemperature();
  float humidity = sht21.getHumidity();

  if ( (celsius != -46.85) && (humidity != -6.0 ) ) { // if sensor reading is valid calculate offsets
    celsius += static_cast<float>(d.sensor.sht21.offsetTemp) / 10.0;
    humidity += static_cast<float>(d.sensor.sht21.offsetHumidity) / 10.0;
  }
  mqttClient.publish( (topic + "temperature").c_str() , String( celsius ).c_str() );
  mqttClient.publish( (topic + "humidity").c_str() , String( humidity ).c_str() );
}

void AM2H_Sht21::config(AM2H_Datastore& d, const MqttTopic& t, const String p){
    AM2H_Core::debugMessage("AM2H_Sht21::config()","old config state {"+String(d.config,BIN)+"}\n");

    if (t.meas_ == "addr") {
        d.sensor.sht21.addr=AM2H_Helper::parse_hex<uint32_t>(p);
        AM2H_Core::debugMessage("AM2H_Sht21::config()","set addr = 0x"+String(d.sensor.sht21.addr,HEX));
        d.config |= Config::SET_0;
    }
    if (t.meas_ == "loc") {
        AM2H_Helper::parse_location(d.loc,p);
        AM2H_Core::debugMessage("AM2H_Sht21::config()","set loc = "+String(d.loc));
        d.config |= Config::SET_1;
    }
    if (t.meas_ == "offsetTemp") {
        d.sensor.sht21.offsetTemp=p.toInt();
        AM2H_Core::debugMessage("AM2H_Sht21::config()","set offsetTemp = "+String(d.sensor.sht21.offsetTemp));
        d.config |= Config::SET_2;
    }
    if (t.meas_ == "offsetHumidity") {
        d.sensor.sht21.offsetHumidity=p.toInt();
        AM2H_Core::debugMessage("AM2H_Sht21::config()","set offsetHumidity = "+String(d.sensor.sht21.offsetHumidity));
        d.config |= Config::SET_3;
    }
    if ( d.config == Config::CHECK_TO_3 ){
        // d.plugin = plugin_;
        d.self = this;
        AM2H_Core::debugMessage("AM2H_Sht21::config()","finished, new config state {"+String(d.config,BIN)+"}");
    } else {
        d.self=nullptr;
    }
}

void AM2H_Sht21::tcaselect(uint8_t i){
    if (i > 7) return;
    Wire.beginTransmission(TCAADDR);
    Wire.write(1 << i);
    Wire.endTransmission();
};

void AM2H_Sht21::scan() {
    for (uint8_t t = 0; t < 7; t++) {
        tcaselect(t);
        AM2H_Core::debugMessage("AM2H_Sht21::scan()", "TCA Port #" + String(t) + "\n");
        for (uint8_t addr = 0; addr <= 127; addr++) {
            if (addr == TCAADDR) continue;
            Wire.beginTransmission(addr);
            int response = Wire.endTransmission();
            if (response == 0) {
                AM2H_Core::debugMessage("AM2H_Sht21::scan()","Found I2C 0x" + String(addr, HEX) + "\n");
            }
        }
    }
}