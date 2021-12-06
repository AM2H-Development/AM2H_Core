#include "AM2H_Sht21.h"
#include "AM2H_Core.h"
#include "libs/SHT21/SHT21.h"

extern AM2H_Core* am2h_core;

void AM2H_Sht21::setupPlugin(int datastoreIndex){
  AM2H_Core::debugMessage("\nSht21::Setup\n");
  scan();
}

void AM2H_Sht21::timerPublish(AM2H_Datastore& d, PubSubClient& mqttClient, const String topic){
  AM2H_Core::debugMessage("Publish: ");

  float celsius = sht21.getTemperature();
  float humidity = sht21.getHumidity();

  if ( (celsius != -46.85) && (humidity != -6.0 ) ) { // if sensor reading is valid calculate offsets
    celsius += ( (float) d.sensor.sht21.offsetTemp / 10.0);
    humidity += ( (float) d.sensor.sht21.offsetHumidity / 10.0);
  }
  mqttClient.publish( (topic + "temperature").c_str() , String( celsius ).c_str() );
  mqttClient.publish( (topic + "humidity").c_str() , String( humidity ).c_str() );
}

void AM2H_Sht21::config(AM2H_Datastore& d, const MqttTopic& t, const String p){
    AM2H_Core::debugMessage("Sht21::Config (" + String(d.config,BIN) + "):");
    AM2H_Core::debugMessage(p);
    AM2H_Core::debugMessage("\n");
    if (t.meas_ == "addr") {
      AM2H_Core::debugMessage("Addr = ");
      d.sensor.sht21.addr=p.toInt();
      AM2H_Core::debugMessage(String(d.sensor.sht21.addr,HEX));
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
    if (t.meas_ == "offsetTemp") {
        d.sensor.sht21.offsetTemp=p.toInt();
        d.config |= Config::SET_2;
    }
    if (t.meas_ == "offsetHumidity") {
        d.sensor.sht21.offsetHumidity=p.toInt();
        d.config |= Config::SET_3;
    }
    if ( d.config == Config::CHECK_TO_3 ){
        d.plugin = plugin_;
        d.self = this;
        AM2H_Core::debugMessage("Config finished\n");
    } else {
        d.self=nullptr;
    }
}


void AM2H_Sht21::tcaselect(uint8_t i){
    if (i > 7) return;
    Wire.beginTransmission(TCAADDR);
    Wire.write(1 << i);
    Wire.endTransmission();
    AM2H_Core::debugMessage("Select Channel " + String(i) + "\n");
};

void AM2H_Sht21::scan() {
    for (uint8_t t = 0; t < 7; t++) {
        tcaselect(t);
        AM2H_Core::debugMessage("TCA Port #" + String(t) + "\n");
        for (uint8_t addr = 0; addr <= 127; addr++) {
            if (addr == TCAADDR) continue;
            Wire.beginTransmission(addr);
            int response = Wire.endTransmission();
            if (response == 0) {
                AM2H_Core::debugMessage("Found I2C 0x" + String(addr, HEX) + "\n");
            }
        }
    }
}
