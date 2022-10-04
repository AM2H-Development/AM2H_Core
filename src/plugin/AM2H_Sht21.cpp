#include "AM2H_Sht21.h"
#include "AM2H_Core.h"
#include "include/AM2H_Helper.h"
#include "libs/SHT21/SHT21.h"

extern AM2H_Core* am2h_core;

void AM2H_Sht21::timerPublish(AM2H_Datastore& d, PubSubClient& mqttClient, const String topic, const uint8_t index){
    am2h_core->switchWire(d.sensor.sht21.addr);
    float celsius = sht21.getTemperature();
    float humidity = sht21.getHumidity();
    char error[] = "1";
    if ( (celsius != -46.85) && (humidity != -6.0 ) ) { // if sensor reading is valid calculate offsets
        AM2H_Core::debugMessageNl("AM2H_Sht21::timerPublish()","publishing " + topic + String(celsius) + " / " + String (humidity), DebugLogger::INFO);
        error[0] = '0';
        celsius += static_cast<float>(d.sensor.sht21.offsetTemp) / 10.0;
        humidity += static_cast<float>(d.sensor.sht21.offsetHumidity) / 10.0;
        mqttClient.publish( (topic + "temperature").c_str() , String( celsius ).c_str() );
        am2h_core->loopMqtt();
        mqttClient.publish( (topic + "humidity").c_str() , String( humidity ).c_str() );
        am2h_core->loopMqtt();
    } else {
        AM2H_Core::debugMessage("AM2H_Sht21::timerPublish()","Error " + String(error), DebugLogger::ERROR);
        am2h_core->i2cReset();
    }
    mqttClient.publish( (topic + ERROR_CODE + "_" + String(index)).c_str(), error );
    am2h_core->loopMqtt();
}

void AM2H_Sht21::config(AM2H_Datastore& d, const MqttTopic& t, const String p){
    AM2H_Core::debugMessageNl("AM2H_Sht21::config()","old config state {"+String(d.config,BIN)+"}", DebugLogger::INFO);

    if (t.meas_ == "addr") {
        d.sensor.sht21.addr=AM2H_Helper::parse_hex<uint32_t>(p);
        AM2H_Core::debugMessage("AM2H_Sht21::config()"," set addr = 0x"+String(d.sensor.sht21.addr,HEX), DebugLogger::INFO);
        d.config |= Config::SET_0;
    }
    if (t.meas_ == "loc") {
        d.config &= ~Config::SET_1;
        if (p.length()>0) {
            AM2H_Helper::parse_location(d.loc,p);
            AM2H_Core::debugMessage("AM2H_Sht21::config()"," set loc = "+String(d.loc), DebugLogger::INFO);
            d.config |= Config::SET_1;
        }
    }
    if (t.meas_ == "offsetTemp") {
        d.sensor.sht21.offsetTemp=p.toInt();
        AM2H_Core::debugMessage("AM2H_Sht21::config()"," set offsetTemp = "+String(d.sensor.sht21.offsetTemp), DebugLogger::INFO);
        d.config |= Config::SET_2;
    }
    if (t.meas_ == "offsetHumidity") {
        d.sensor.sht21.offsetHumidity=p.toInt();
        AM2H_Core::debugMessage("AM2H_Sht21::config()"," set offsetHumidity = "+String(d.sensor.sht21.offsetHumidity), DebugLogger::INFO);
        d.config |= Config::SET_3;
    }
    if ( d.config == Config::CHECK_TO_3 ){
        // d.plugin = plugin_;
        d.self = this;
        AM2H_Core::debugMessageNl("AM2H_Sht21::config()","finished, new config state {"+String(d.config,BIN)+"}", DebugLogger::INFO);
    } else {
        d.self=nullptr;
    }
}