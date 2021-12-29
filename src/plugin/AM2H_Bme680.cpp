#include "AM2H_Bme680.h"
#include "AM2H_Core.h"
#include "libs/BSEC_Software_Library/src/bsec.h"

extern AM2H_Core* am2h_core;

void AM2H_Bme680::loopPlugin(AM2H_Datastore& d){
    Bsec& bme680 = *d.sensor.bme680.bme680;
    bme680.run();
}

void AM2H_Bme680::timerPublish(AM2H_Datastore& d, PubSubClient& mqttClient, const String topic){
    Bsec& bme680 = *d.sensor.bme680.bme680;
    AM2H_Core::debugMessage("AM2H_Bme680::timerPublish()","publishing to " + topic);

    mqttClient.publish( (topic + "temperature").c_str() , String( bme680.temperature+( static_cast<float>(d.sensor.bme680.offsetTemp) / 10.0)).c_str() );
    mqttClient.publish( (topic + "humidity").c_str() , String( bme680.humidity+ ( static_cast<float>(d.sensor.bme680.offsetHumidity) / 10.0) ).c_str() );
    mqttClient.publish( (topic + "pressure").c_str() , String( bme680.pressure/100. + ( static_cast<float>(d.sensor.bme680.offsetPressure) / 10.0) ).c_str() );

    mqttClient.publish( (topic + "co2Accuracy").c_str() , String( bme680.co2Accuracy ).c_str() );    
    if ( bme680.co2Accuracy>=1 ) {
        mqttClient.publish( (topic + "co2Equivalent").c_str() , String( bme680.co2Equivalent ).c_str() );
    }

    mqttClient.publish( (topic + "iaqAccuracy").c_str() , String( bme680.iaqAccuracy ).c_str() );
    if ( bme680.iaqAccuracy>=1 ) {
        mqttClient.publish( (topic + "iaq").c_str() , String( bme680.iaq ).c_str() );
    }

    mqttClient.publish( (topic + "staticIaqAccuracy").c_str() , String( bme680.staticIaqAccuracy ).c_str() );
    if ( bme680.staticIaqAccuracy>=1 ) {
        mqttClient.publish( (topic + "staticIaq").c_str() , String( bme680.staticIaq ).c_str() );
    }

    if ( bme680.staticIaqAccuracy>=3 ) {
        uint8_t bsecState[BSEC_MAX_STATE_BLOB_SIZE] {0};
        bme680.getState(bsecState);
        AM2H_Core::debugMessage("AM2H_Bme680::timerPublish()","saving IAQ\n");
        mqttClient.publish( d.sensor.bme680.iaqConfigTopic , bsecState, BSEC_MAX_STATE_BLOB_SIZE, true );
    }
}

void AM2H_Bme680::config(AM2H_Datastore& d, const MqttTopic& t, const String p){
    AM2H_Core::debugMessage("AM2H_Bme680::config()","old config state {"+String(d.config,BIN)+"}\n");
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

    if (t.meas_ == "addr") {
        d.sensor.bme680.addr=AM2H_Helper::parse_hex<uint32_t>(p);
        AM2H_Core::debugMessage("AM2H_Bme680::config()","set addr = 0x"+String(d.sensor.bme680.addr,HEX));
        d.config |= Config::SET_0;
    }
    if (t.meas_ == "loc") {
        AM2H_Helper::parse_location(d.loc,p);
        AM2H_Core::debugMessage("AM2H_Bme680::config()","set loc = "+String(d.loc));
        d.config |= Config::SET_1;
    }
    if (t.meas_ == "offsetTemp") {
        d.sensor.bme680.offsetTemp=p.toInt();
        AM2H_Core::debugMessage("AM2H_Bme680::config()","set offsetTemp = "+String(d.sensor.bme680.offsetTemp));
        d.config |= Config::SET_2;
    }
    if (t.meas_ == "offsetHumidity") {
        d.sensor.bme680.offsetHumidity=p.toInt();
        AM2H_Core::debugMessage("AM2H_Bme680::config()","set offsetHumidity = "+String(d.sensor.bme680.offsetHumidity));
        d.config |= Config::SET_3;
    }
    if (t.meas_ == "offsetPressure") {
        d.sensor.bme680.offsetHumidity=p.toInt();
        AM2H_Core::debugMessage("AM2H_Bme680::config()","set offsetPressure = "+String(d.sensor.bme680.offsetPressure));
        d.config |= Config::SET_4;
    }
    if (t.meas_ == "setIAQ") {
        AM2H_Core::debugMessage("AM2H_Bme680::config()","attempt to set IAQ when iaqStateSetReady==false: iaqStateSetReady is" + String(d.sensor.bme680.iaqStateSetReady ? " true":" false") );
        if ( !d.sensor.bme680.iaqStateSetReady ) { // set saved IAQ data only once
            AM2H_Core::debugMessage("AM2H_Bme680::config()"," set!");
            set_iaq(d,p);
        }
        d.config |= Config::SET_5;
    }
    if ( d.config == Config::CHECK_TO_5 ){
        // d.plugin = plugin_;
        d.self = this;
        if ( !d.sensor.bme680.iaqStateSetReady ) { // apply post config and set saved IAQ data only once
            d.sensor.bme680.iaqStateSetReady = true;
            postConfig(d);
        }
        AM2H_Core::debugMessage("AM2H_Bme680::config()"," finished, new config state {"+String(d.config,BIN)+"}");
    } else {
        d.self=nullptr;
    }
}

void AM2H_Bme680::postConfig(AM2H_Datastore& d){
    Bsec& bme680 = *d.sensor.bme680.bme680;

    bme680.begin(d.sensor.bme680.addr, Wire);
    AM2H_Core::debugMessage("AM2H_Bme680::postConfig()","begin() BSEC-Status:" + String(bme680.status) + " BME-Status" + String(bme680.bme680Status)+"\n");
    bme680.setConfig(BSEC_CONFIG_IAQ);
    AM2H_Core::debugMessage("AM2H_Bme680::postConfig()","setConfig() BSEC-Status:" + String(bme680.status) + " BME-Status" + String(bme680.bme680Status)+"\n");

    if ( d.sensor.bme680.iaqStateSave!=nullptr ){
        bme680.setState(d.sensor.bme680.iaqStateSave);
        AM2H_Core::debugMessage("AM2H_Bme680::postConfig()","setState() BSEC-Status:" + String(bme680.status) + " BME-Status" + String(bme680.bme680Status)+"\n");
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
    AM2H_Core::debugMessage("AM2H_Bme680::postConfig()","updateSubscription() BSEC-Status:" + String(bme680.status) + " BME-Status" + String(bme680.bme680Status)+"\n");
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