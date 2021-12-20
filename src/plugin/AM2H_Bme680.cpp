#include "AM2H_Bme680.h"
#include "AM2H_Core.h"
#include "libs/BSEC_Software_Library/src/bsec.h"

extern AM2H_Core* am2h_core;

void AM2H_Bme680::setupPlugin(int datastoreIndex){
    AM2H_Core::debugMessage("\nBme680::Setup\n");
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
}

void AM2H_Bme680::config(AM2H_Datastore& d, const MqttTopic& t, const String p){
    AM2H_Core::debugMessage("Bme680::Config (" + String(d.config,BIN) + "):");
    AM2H_Core::debugMessage(p);
    AM2H_Core::debugMessage("\n");
    if (t.meas_ == "addr") {
      AM2H_Core::debugMessage("Addr = ");
      d.sensor.bme680.addr=p.toInt();
      AM2H_Core::debugMessage(String(d.sensor.bme680.addr,HEX));
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
        d.sensor.bme680.offsetTemp=p.toInt();
        d.config |= Config::SET_2;
    }
    if (t.meas_ == "offsetHumidity") {
        d.sensor.bme680.offsetHumidity=p.toInt();
        d.config |= Config::SET_3;
    }
    if ( d.config == Config::CHECK_TO_3 ){
        d.plugin = plugin_;
        d.self = this;
        bme680.begin(d.sensor.bme680.addr, Wire);
        bme680.setConfig(bsec_config_iaq);

        // todo loadState from MQTT (formerly EEPROM)

        constexpr int SENSORS{3};
        bsec_virtual_sensor_t sensorList[SENSORS] = {
            BSEC_OUTPUT_IAQ,
            BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE,
            BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY,
        };

        bme680.updateSubscription(sensorList, SENSORS, BSEC_SAMPLE_RATE_LP);

        AM2H_Core::debugMessage("Config finished\n");
    } else {
        d.self=nullptr;
    }
}
