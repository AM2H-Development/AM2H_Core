#include "AM2H_Bme680.h"
#include "AM2H_Core.h"
#include "libs/BSEC_Software_Library/src/bsec.h"

extern AM2H_Core* am2h_core;
extern AM2H_Datastore ds[20];

void AM2H_Bme680::setupPlugin(int datastoreIndex){
    AM2H_Core::debugMessage("\nBme680::Setup\n");
    ds[datastoreIndex].sensor.bme680.iaq=nullptr;
}

void AM2H_Bme680::loopPlugin(int datastoreIndex){
    if (bme680.run()) { // If new data is available
        // output += ", " + String(iaqSensor.pressure);
        // output += ", " + String(iaqSensor.iaq);
        // output += ", " + String(iaqSensor.iaqAccuracy);
        temperature = bme680.temperature;
        humidity = bme680.humidity;
    }
}

void AM2H_Bme680::timerPublish(AM2H_Datastore& d, PubSubClient& mqttClient, const String topic){
    AM2H_Core::debugMessage("Publish: ");

    temperature += ( (float) d.sensor.bme680.offsetTemp / 10.0);
    humidity += ( (float) d.sensor.bme680.offsetHumidity / 10.0);

    mqttClient.publish( (topic + "temperature").c_str() , String( temperature ).c_str() );
    mqttClient.publish( (topic + "humidity").c_str() , String( humidity ).c_str() );
    // mqttClient.publish( (topic + "iaqsave").c_str() , d.sensor.bme680.iaq, 2048, true );
}

void AM2H_Bme680::config(AM2H_Datastore& d, const MqttTopic& t, const String p){
    AM2H_Core::debugMessage("Bme680::Config (" + String(d.config,BIN) + "):");
    AM2H_Core::debugMessage(p);
    AM2H_Core::debugMessage("\n");
    if (t.meas_ == "addr") {
      d.sensor.bme680.addr=p.toInt();
      AM2H_Core::debugMessage("Addr = 0x"+String(d.sensor.bme680.addr,HEX)+"\n");
      d.config |= Config::SET_0;
    }
    if (t.meas_ == "loc") {
        AM2H_Helper::parse_location(d.loc,p);
        d.config |= Config::SET_1;
    }
    if (t.meas_ == "offsetTemp") {
        d.sensor.bme680.offsetTemp=p.toInt();
        d.config |= Config::SET_2;
    }
    if (t.meas_ == "offsetHumidity") {
        d.sensor.bme680.offsetHumidity=p.toInt();
        d.config |= Config::SET_3;
    }
    if (t.meas_ == "setIAQ") {
        set_iaq(d,p);
        d.config |= Config::SET_4;
    }
    if ( d.config == Config::CHECK_TO_4 ){
        d.plugin = plugin_;
        d.self = this;
        postConfig(d);
        AM2H_Core::debugMessage("Config finished\n");
    } else {
        d.self=nullptr;
    }
}

void AM2H_Bme680::postConfig(AM2H_Datastore& d){
    bme680.begin(d.sensor.bme680.addr, Wire);

    if ( d.sensor.bme680.iaq==nullptr ){
        bme680.setConfig(BSEC_CONFIG_IAQ);
    } else {
        bme680.setConfig(d.sensor.bme680.iaq);
        delete [] d.sensor.bme680.iaq;
        d.sensor.bme680.iaq=nullptr;
    }

    constexpr int SENSORS{3};
    bsec_virtual_sensor_t sensorList[SENSORS] = {
        BSEC_OUTPUT_IAQ,
        BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE,
        BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY,
    };

    bme680.updateSubscription(sensorList, SENSORS, BSEC_SAMPLE_RATE_LP);
}

void AM2H_Bme680::set_iaq(AM2H_Datastore& d, const String p){
    if (p.length()<10){return;}
    if ( d.sensor.bme680.iaq == nullptr ){
        d.sensor.bme680.iaq = new uint8_t[BSEC_MAX_WORKBUFFER_SIZE];
    }
    size_t i{0};
    for (auto c : p){
       d.sensor.bme680.iaq[i++]=c;
       if (i>=BSEC_MAX_WORKBUFFER_SIZE) {break;} 
    }
}