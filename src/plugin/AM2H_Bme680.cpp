#include "AM2H_Bme680.h"
#include "AM2H_Core.h"
#include "include/AM2H_Helper.h"
#include "libs/BSEC_Software_Library/src/bsec.h"

extern AM2H_Core* am2h_core;

void AM2H_Bme680::loopPlugin(AM2H_Datastore& d, const uint8_t index){
    Bsec& bme680 = *d.sensor.bme680.bme680;
    am2h_core->switchWire(d.sensor.bme680.addr);
    bme680.run();
}

void AM2H_Bme680::timerPublish(AM2H_Datastore& d, PubSubClient& mqttClient, const String topic, const uint8_t index){
    Bsec& bme680 = *d.sensor.bme680.bme680;
    AM2H_Core::debugMessageNl("AM2H_Bme680::timerPublish()","publishing to " + topic, DebugLogger::INFO);

    mqttClient.publish( (topic + ERROR_CODE + "_" + String(index)).c_str() , String(bme680.bme680Status).c_str() );
    am2h_core->loopMqtt();

    if ( bme680.bme680Status == BSEC_OK ) {
        mqttClient.publish( (topic + "temperature").c_str() , String( bme680.temperature+( static_cast<float>(d.sensor.bme680.offsetTemp) / 10.0)).c_str() );
        am2h_core->loopMqtt();

        mqttClient.publish( (topic + "temperature").c_str() , String( bme680.temperature+( static_cast<float>(d.sensor.bme680.offsetTemp) / 10.0)).c_str() );
        am2h_core->loopMqtt();

        mqttClient.publish( (topic + "humidity").c_str() , String( bme680.humidity+ ( static_cast<float>(d.sensor.bme680.offsetHumidity) / 10.0) ).c_str() );
        am2h_core->loopMqtt();

        mqttClient.publish( (topic + "pressure").c_str() , String( bme680.pressure/100. + ( static_cast<float>(d.sensor.bme680.offsetPressure) / 10.0) ).c_str() );
        am2h_core->loopMqtt();

        mqttClient.publish( (topic + "co2Accuracy").c_str() , String( bme680.co2Accuracy ).c_str() );    
        am2h_core->loopMqtt();

        if ( bme680.co2Accuracy>=1 ) {
            mqttClient.publish( (topic + "co2Equivalent").c_str() , String( bme680.co2Equivalent ).c_str() );
            am2h_core->loopMqtt();
        }

        mqttClient.publish( (topic + "iaqAccuracy").c_str() , String( bme680.iaqAccuracy ).c_str() );
        am2h_core->loopMqtt();
        if ( bme680.iaqAccuracy>=1 ) {
            mqttClient.publish( (topic + "iaq").c_str() , String( bme680.iaq ).c_str() );
            am2h_core->loopMqtt();
        }

        mqttClient.publish( (topic + "staticIaqAccuracy").c_str() , String( bme680.staticIaqAccuracy ).c_str() );
        am2h_core->loopMqtt();
        if ( bme680.staticIaqAccuracy>=1 ) {
            mqttClient.publish( (topic + "staticIaq").c_str() , String( bme680.staticIaq ).c_str() );
            am2h_core->loopMqtt();
        }

        if ( bme680.staticIaqAccuracy>=1 ) {
            uint8_t bsecState[BSEC_MAX_STATE_BLOB_SIZE] {0};
            bme680.getState(bsecState);
            AM2H_Core::debugMessage("AM2H_Bme680::timerPublish()","saving IAQ\n", DebugLogger::INFO);
            mqttClient.publish( d.sensor.bme680.iaqConfigTopic , bsecState, BSEC_MAX_STATE_BLOB_SIZE, true );
            am2h_core->loopMqtt();
        }
    } else {
        AM2H_Core::debugMessage("AM2H_Bme680::timerPublish()","Error " + String(bme680.bme680Status), DebugLogger::ERROR);
        am2h_core->i2cReset();
    }
}

void AM2H_Bme680::config(AM2H_Datastore& d, const MqttTopic& t, const String p){
    AM2H_Core::debugMessageNl("AM2H_Bme680::config()","old config state {"+String(d.config,BIN)+"}", DebugLogger::INFO);
    if (!d.initialized) {
        d.initialized=true;

        String tempTopic = am2h_core->getFullStorageTopic(String(t.id_), t.srv_, "setIAQ");
        d.sensor.bme680.iaqConfigTopic = new char[tempTopic.length()];
        size_t i=0;
        for (auto c: tempTopic ){
            d.sensor.bme680.iaqConfigTopic[i++]=c;
        }
        d.sensor.bme680.iaqConfigTopic[i]='\0';
        
        am2h_core->subscribe(d.sensor.bme680.iaqConfigTopic);
        d.sensor.bme680.bme680 = new Bsec();
        d.sensor.bme680.iaqStateSetReady=false;
    }

    if (t.meas_ == "addr") {
        d.sensor.bme680.addr=AM2H_Helper::parse_hex<uint32_t>(p);
        AM2H_Core::debugMessage("AM2H_Bme680::config()"," set addr = 0x"+String(d.sensor.bme680.addr,HEX), DebugLogger::INFO);
        d.config |= Config::SET_0;
    }
    if (t.meas_ == "loc") {
        d.config &= ~Config::SET_1;
        if (p.length()>0) {
            AM2H_Helper::parse_location(d.loc,p);
            AM2H_Core::debugMessage("AM2H_Bme680::config()"," set loc = "+String(d.loc), DebugLogger::INFO);
            d.config |= Config::SET_1;
        }
    }
    if (t.meas_ == "offsetTemp") {
        d.sensor.bme680.offsetTemp=p.toInt();
        AM2H_Core::debugMessage("AM2H_Bme680::config()"," set offsetTemp = "+String(d.sensor.bme680.offsetTemp), DebugLogger::INFO);
        d.config |= Config::SET_2;
    }
    if (t.meas_ == "offsetHumidity") {
        d.sensor.bme680.offsetHumidity=p.toInt();
        AM2H_Core::debugMessage("AM2H_Bme680::config()"," set offsetHumidity = "+String(d.sensor.bme680.offsetHumidity), DebugLogger::INFO);
        d.config |= Config::SET_3;
    }
    if (t.meas_ == "offsetPressure") {
        d.sensor.bme680.offsetHumidity=p.toInt();
        AM2H_Core::debugMessage("AM2H_Bme680::config()"," set offsetPressure = "+String(d.sensor.bme680.offsetPressure), DebugLogger::INFO);
        d.config |= Config::SET_4;
    }
    if (t.meas_ == "setIAQ") {
        AM2H_Core::debugMessage("AM2H_Bme680::config()"," attempt to set IAQ when iaqStateSetReady==false: iaqStateSetReady is" + String(d.sensor.bme680.iaqStateSetReady ? " true":" false"), DebugLogger::INFO);
        if ( !d.sensor.bme680.iaqStateSetReady ) { // set saved IAQ data only once
            AM2H_Core::debugMessage("AM2H_Bme680::config()"," set!", DebugLogger::INFO);
            set_iaq(d,p);
        }
        am2h_core->unsubscribe(d.sensor.bme680.iaqConfigTopic);
        d.config |= Config::SET_5;
    }
    if ( d.config == Config::CHECK_TO_5 ){
        // d.plugin = plugin_;
        d.self = this;
        if ( !d.sensor.bme680.iaqStateSetReady ) { // apply post config and set saved IAQ data only once
            d.sensor.bme680.iaqStateSetReady = true;
            postConfig(d);
        }
        AM2H_Core::debugMessageNl("AM2H_Bme680::config()"," finished, new config state {"+String(d.config,BIN)+"}", DebugLogger::INFO);
    } else {
        d.self=nullptr;
    }
}

void AM2H_Bme680::postConfig(AM2H_Datastore& d){
    Bsec& bme680 = *d.sensor.bme680.bme680;

    am2h_core->switchWire(d.sensor.bme680.addr);
    bme680.begin(d.sensor.bme680.addr, Wire);
    AM2H_Core::debugMessage("AM2H_Bme680::postConfig()","begin() BSEC-Status:" + String(bme680.status) + " BME-Status" + String(bme680.bme680Status), DebugLogger::INFO);
    bme680.setConfig(BSEC_CONFIG_IAQ);
    AM2H_Core::debugMessage("AM2H_Bme680::postConfig()","setConfig() BSEC-Status:" + String(bme680.status) + " BME-Status" + String(bme680.bme680Status), DebugLogger::INFO);

    if ( d.sensor.bme680.iaqStateSave!=nullptr ){
        bme680.setState(d.sensor.bme680.iaqStateSave);
        AM2H_Core::debugMessage("AM2H_Bme680::postConfig()","setState() BSEC-Status:" + String(bme680.status) + " BME-Status" + String(bme680.bme680Status), DebugLogger::INFO);
        delete [] d.sensor.bme680.iaqStateSave;
        d.sensor.bme680.iaqStateSave=nullptr;
    }
    constexpr int SENSORS{6};
    bsec_virtual_sensor_t sensorList[SENSORS] = {
        BSEC_OUTPUT_IAQ,
        BSEC_OUTPUT_STATIC_IAQ,
        BSEC_OUTPUT_CO2_EQUIVALENT,
        BSEC_OUTPUT_RAW_PRESSURE,
        BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE,
        BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY
    };
    bme680.updateSubscription(sensorList, SENSORS, BSEC_SAMPLE_RATE_LP);
    AM2H_Core::debugMessage("AM2H_Bme680::postConfig()","updateSubscription() BSEC-Status:" + String(bme680.status) + " BME-Status" + String(bme680.bme680Status), DebugLogger::INFO);
}

void AM2H_Bme680::set_iaq(AM2H_Datastore& d, const String p){
    if (p.length()!=BSEC_MAX_STATE_BLOB_SIZE){return;}
    if ( d.sensor.bme680.iaqStateSave == nullptr ){
        d.sensor.bme680.iaqStateSave = new uint8_t[BSEC_MAX_STATE_BLOB_SIZE];
    }
    size_t i{0};
    for (auto c : p){
       d.sensor.bme680.iaqStateSave[i++]=c;
       if (i>=BSEC_MAX_STATE_BLOB_SIZE) {break;}
    }
}