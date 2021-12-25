#include "AM2H_Bme680.h"
#include "AM2H_Core.h"
#include "libs/BSEC_Software_Library/src/bsec.h"

extern AM2H_Core* am2h_core;

void AM2H_Bme680::loopPlugin(AM2H_Datastore& d){
    Bsec& bme680 = *d.sensor.bme680.bme680;
    if (bme680.run()) { // If new data is available
        // output += ", " + String(iaqSensor.pressure);
        // output += ", " + String(iaqSensor.iaq);
        // output += ", " + String(iaqSensor.iaqAccuracy);
        // auto a = bme680.pressure;
        
        // d.sensor.bme680.temperature = bme680.temperature;
        // d.sensor.bme680.humidity = bme680.humidity;
    }
}

void AM2H_Bme680::timerPublish(AM2H_Datastore& d, PubSubClient& mqttClient, const String topic){
    Bsec& bme680 = *d.sensor.bme680.bme680;
    AM2H_Core::debugMessage("Publish: ");

    AM2H_Core::debugMessage("Temp+Hum:" + String(millis())+"\n");
    mqttClient.publish( (topic + "temperature").c_str() , String( bme680.temperature+( (float) d.sensor.bme680.offsetTemp / 10.0)).c_str() );
    mqttClient.publish( (topic + "humidity").c_str() , String( bme680.humidity+ ( (float) d.sensor.bme680.offsetHumidity / 10.0) ).c_str() );
    mqttClient.publish( (topic + "pressure").c_str() , String( bme680.pressure+ ( (float) d.sensor.bme680.offsetPressure / 10.0) ).c_str() );
    mqttClient.publish( (topic + "iaq").c_str() , String( bme680.iaq ).c_str() );
    mqttClient.publish( (topic + "iaqAccuracy").c_str() , String( bme680.iaqAccuracy ).c_str() );

    if ( bme680.staticIaqAccuracy>=3 ) {
        uint8_t bsecState[BSEC_MAX_STATE_BLOB_SIZE] {0};
        bme680.getState(bsecState);
        AM2H_Core::debugMessage("IAQSave:" + String(millis())+"\n");
        mqttClient.publish( d.sensor.bme680.iaqConfigTopic , bsecState, BSEC_MAX_STATE_BLOB_SIZE, true );
    }
}

void AM2H_Bme680::config(AM2H_Datastore& d, const MqttTopic& t, const String p){
    if (!d.initialized) {
        String tempTopic = am2h_core->getConfigTopic()+t.srv_+"/"+String(t.id_)+"/setIAQ";
        d.sensor.bme680.iaqConfigTopic = new char[tempTopic.length()];
        size_t i=0;
        for (auto c: tempTopic ){
            d.sensor.bme680.iaqConfigTopic[i++]=c;
        }
        d.sensor.bme680.iaqConfigTopic[i]='\0';

        d.sensor.bme680.bme680 = new Bsec();
        d.sensor.bme680.iaqStateSetReady=false;
        d.initialized=true;
    }
    AM2H_Core::debugMessage("Bme680::Config (" + String(d.config,BIN) + ")\n");
    if (t.meas_ == "addr") {
      d.sensor.bme680.addr=p.toInt();
      AM2H_Core::debugMessage("Addr = 0x"+String(d.sensor.bme680.addr,HEX)+"\n");
      d.config |= Config::SET_0;
    }
    if (t.meas_ == "loc") {
        AM2H_Core::debugMessage("loc = "+p+"\n");
        AM2H_Helper::parse_location(d.loc,p);
        d.config |= Config::SET_1;
    }
    if (t.meas_ == "offsetTemp") {
        AM2H_Core::debugMessage("oT = "+p+"\n");
        d.sensor.bme680.offsetTemp=p.toInt();
        d.config |= Config::SET_2;
    }
    if (t.meas_ == "offsetHumidity") {
        AM2H_Core::debugMessage("oH = "+p+"\n");
        d.sensor.bme680.offsetHumidity=p.toInt();
        d.config |= Config::SET_3;
    }
    if (t.meas_ == "offsetPressure") {
        AM2H_Core::debugMessage("oP = "+p+"\n");
        d.sensor.bme680.offsetHumidity=p.toInt();
        d.config |= Config::SET_4;
    }
    if (t.meas_ == "setIAQ") {
        AM2H_Core::debugMessage("Bme680::setIAQ (" + String(d.sensor.bme680.iaqStateSetReady) + ")\n");
         if ( !d.sensor.bme680.iaqStateSetReady ) { // set saved IAQ data only once
            AM2H_Core::debugMessage("Bme680::setIAQ2 (" + String(d.sensor.bme680.iaqStateSetReady) + ")\n");
            set_iaq(d,p);
        }
        d.config |= Config::SET_5;
    }
    if ( d.config == Config::CHECK_TO_5 ){
        d.plugin = plugin_;
        d.self = this;
        if ( !d.sensor.bme680.iaqStateSetReady ) { // apply post config and set saved IAQ data only once
            d.sensor.bme680.iaqStateSetReady = true;
            postConfig(d);
        }
        AM2H_Core::debugMessage("Config finished\n");
    } else {
        d.self=nullptr;
    }
}

void AM2H_Bme680::postConfig(AM2H_Datastore& d){
    Bsec& bme680 = *d.sensor.bme680.bme680;

    bme680.begin(d.sensor.bme680.addr, Wire);
    AM2H_Core::debugMessage("begin() BSEC-Status:" + String(bme680.status) + " BME-Status" + String(bme680.bme680Status)+"\n");
    bme680.setConfig(BSEC_CONFIG_IAQ);
    AM2H_Core::debugMessage("setConfig() BSEC-Status:" + String(bme680.status) + " BME-Status" + String(bme680.bme680Status)+"\n");

    if ( d.sensor.bme680.iaqStateSave!=nullptr ){
        bme680.setState(d.sensor.bme680.iaqStateSave);
        AM2H_Core::debugMessage("setState() BSEC-Status:" + String(bme680.status) + " BME-Status" + String(bme680.bme680Status)+"\n");
        delete [] d.sensor.bme680.iaqStateSave;
        d.sensor.bme680.iaqStateSave=nullptr;
    }
    constexpr int SENSORS{4};
    bsec_virtual_sensor_t sensorList[SENSORS] = {
        BSEC_OUTPUT_IAQ,
        BSEC_OUTPUT_RAW_PRESSURE,
        BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE,
        BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY
    };

    bme680.updateSubscription(sensorList, SENSORS, BSEC_SAMPLE_RATE_LP);
    AM2H_Core::debugMessage("updateSubscription() BSEC-Status:" + String(bme680.status) + " BME-Status" + String(bme680.bme680Status)+"\n");
}

void AM2H_Bme680::set_iaq(AM2H_Datastore& d, const String p){
    if (p.length()<10){return;}
    if ( d.sensor.bme680.iaqStateSave == nullptr ){
        d.sensor.bme680.iaqStateSave = new uint8_t[BSEC_MAX_STATE_BLOB_SIZE];
    }
    size_t i{0};
    for (auto c : p){
       d.sensor.bme680.iaqStateSave[i++]=c;
       if (i>=BSEC_MAX_STATE_BLOB_SIZE) {break;}
    }
}

void AM2H_Bme680::update_iaq(AM2H_Datastore& d){
/*    bool update = false;
    if (stateUpdateCounter == 0) {
        if (bme680.iaqAccuracy >= 3) {
            update = true;
            stateUpdateCounter++;
        }
    } else {
        if ((stateUpdateCounter * STATE_SAVE_PERIOD) < millis()) {
            update = true;
            stateUpdateCounter++;
        }
    }

    if (update) {

        bme680.getState(bsecState);
        AM2H_Core::debugMessage("Writing state to MQTT");
    }
    */
}