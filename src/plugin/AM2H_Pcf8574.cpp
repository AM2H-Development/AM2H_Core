#include "AM2H_Pcf8574.h"
#include "AM2H_Core.h"
#include "include/AM2H_Helper.h"
#include <Wire.h>

static_assert( PCF8574_MIN_RATE_MS > PCF8574_WAIT_READ_MS, "PCF8574_MIN_RATE_MS must be bigger than PCF8574_WAIT_READ_MS");

extern AM2H_Core* am2h_core;

void AM2H_Pcf8574::loopPlugin(AM2H_Datastore& d, const uint8_t index){
}

void AM2H_Pcf8574::timerPublish(AM2H_Datastore& d, PubSubClient& mqttClient, const String topic, const uint8_t index){
    const auto& pcf8574=d.sensor.pcf8574;
    String config = "ioMask="+AM2H_Helper::formatBinary8(pcf8574.ioMask)+"&reg="+AM2H_Helper::formatBinary8(pcf8574.reg);
    AM2H_Core::debugMessageNl("AM2H_Pcf8574::timerPublish()","publishing to " + topic + "state: " + config, DebugLogger::INFO);
    mqttClient.publish( (topic + "state").c_str() , config.c_str() );
    am2h_core->loopMqtt();
}

void AM2H_Pcf8574::interruptPublish(AM2H_Datastore& d, PubSubClient& mqttClient, const String topic, const uint8_t index){
    AM2H_Core::debugMessageNl("AM2H_Pcf8574::interruptPublish()","calling timerPublish() ", DebugLogger::INFO);
    delay( PCF8574_WAIT_READ_MS );
    readReg(d);
    timerPublish(d,mqttClient,topic,index);
    uint8_t state=d.sensor.pcf8574.reg;
    delay( PCF8574_MIN_RATE_MS - PCF8574_WAIT_READ_MS );
    readReg(d);
    if (state != d.sensor.pcf8574.reg) {
        timerPublish(d,mqttClient,topic,index);
    }
}

void AM2H_Pcf8574::config(AM2H_Datastore& d, const MqttTopic& t, const String p){
    AM2H_Core::debugMessageNl("AM2H_Pcf8574::config()","old config state {"+String(d.config,BIN)+"}", DebugLogger::INFO);
    if (t.meas_ == "addr") {
        d.sensor.pcf8574.addr= AM2H_Helper::parse_hex<uint32_t>(p);
        AM2H_Core::debugMessage("AM2H_Pcf8574::config()"," set addr = 0x"+String(d.sensor.pcf8574.addr,HEX), DebugLogger::INFO);
        d.config |= Config::SET_0;
    }
    if (t.meas_ == "loc") {
        d.config &= ~Config::SET_1;
        if (p.length()>0) {
            AM2H_Helper::parse_location(d.loc,p);
            AM2H_Core::debugMessage("AM2H_Pcf8574::config()"," set loc = "+String(d.loc), DebugLogger::INFO);
            d.config |= Config::SET_1;
        }
    }
    if (t.meas_ == "ioMask") {
        d.sensor.pcf8574.ioMask=p.toInt();
        AM2H_Core::debugMessage("AM2H_Pcf8574::config()"," set ioMask = "+AM2H_Helper::formatBinary8(d.sensor.pcf8574.ioMask), DebugLogger::INFO);
        d.config |= Config::SET_2;
    }
    if (t.meas_ == "reg") { // must be native
        d.sensor.pcf8574.reg=p.toInt();
        AM2H_Core::debugMessage("AM2H_Pcf8574::config()"," set reg = "+AM2H_Helper::formatBinary8(d.sensor.pcf8574.reg), DebugLogger::INFO);
        d.config |= Config::SET_3;
    }
    if ( d.config == Config::CHECK_TO_3 ){
        d.self = this;
        if (!d.initialized){
            d.initialized=true;
            updateReg(d);
        }
        AM2H_Core::debugMessageNl("AM2H_Pcf8574::config()","finished, new config state {"+String(d.config,BIN)+"}", DebugLogger::INFO);
    } else {
        d.self=nullptr;
    }

    if ( d.initialized && (t.meas_ == "setState") ) {
        AM2H_Core::debugMessage("AM2H_Pcf8574::config()","setState", DebugLogger::INFO);
        processSetStateTopic(d,p);
        updateReg(d);
        timerPublish(d, am2h_core->getMqttClient(), am2h_core->getDataTopic(d.loc,getSrv(),String(t.id_)),0 );
        return;
    }
    if ( d.initialized && (t.meas_ == "setPort")) {
        processSetPortTopic(d,p);
        updateReg(d);
        AM2H_Core::debugMessage("AM2H_Pcf8574::config()","setPort reg = "+AM2H_Helper::formatBinary8(d.sensor.pcf8574.reg), DebugLogger::INFO);
        timerPublish(d, am2h_core->getMqttClient(), am2h_core->getDataTopic(d.loc,getSrv(),String(t.id_)),0 );
    }
}

void AM2H_Pcf8574::updateReg(AM2H_Datastore& d){
    am2h_core->switchWire(d.sensor.pcf8574.addr);
    Wire.beginTransmission(static_cast<uint8_t>(d.sensor.pcf8574.addr));
    uint8_t out = d.sensor.pcf8574.reg | ~d.sensor.pcf8574.ioMask;
    AM2H_Core::debugMessage("AM2H_Pcf8574::updateReg()","sending " + String(out), DebugLogger::INFO);
    Wire.write(d.sensor.pcf8574.reg | ~d.sensor.pcf8574.ioMask);
    if ( Wire.endTransmission() != 0 ) {
        AM2H_Core::debugMessage("AM2H_Pcf8574::updateReg()","error while transmitting", DebugLogger::ERROR);
        return;
    };
}

void AM2H_Pcf8574::readReg(AM2H_Datastore& d){
    am2h_core->switchWire(d.sensor.pcf8574.addr);
    uint8_t num_bytes{0};
    num_bytes = Wire.requestFrom(static_cast<uint8_t>(d.sensor.pcf8574.addr),1);
    if ( num_bytes != 1 ) {
        AM2H_Core::debugMessage("AM2H_Pcf8574::readReg","error while receiving", DebugLogger::ERROR);
        return;
    }
    d.sensor.pcf8574.reg = Wire.read();
}

void AM2H_Pcf8574::processSetStateTopic(AM2H_Datastore& d,const String& p_origin){
    auto& pcf8574=d.sensor.pcf8574;
    String p=p_origin+"&";
    String key{};
    String value{};
    bool isKey{true};

    for (auto c: p){
        if (c=='='){
            isKey=false;
            continue;
        }
        if (c=='&'){
            isKey=true;
            if ( key.equalsIgnoreCase("ioMask") ) { pcf8574.ioMask = value.toInt(); }
            if ( key.equalsIgnoreCase("reg") ) { pcf8574.reg = value.toInt(); }
            AM2H_Core::debugMessage("AM2H_Pcf8574::parseStateTopic()","key:value = "+key+":"+value, DebugLogger::INFO);
            key="";
            value="";
            continue;
        }
        if (isKey) { key+=c; } else { value+=c; }
    }
}

void AM2H_Pcf8574::processSetPortTopic(AM2H_Datastore& d,const String& p_origin){
    auto& pcf8574=d.sensor.pcf8574;
    String p=p_origin+"&";
    String key{};
    String value{};
    bool isKey{true};
    uint8_t port{0xFF};
    uint8_t state{0xFF};

    for (auto c: p) {
        if (c=='='){
            isKey=false;
            continue;
        }
        if (c=='&'){
            isKey=true;
            if ( key.equalsIgnoreCase("port") ) {
                const long t_port = value.toInt();
                if ( (t_port >= 0) && (t_port <= 7) ) { port=t_port;}
            }
            if ( key.equalsIgnoreCase("state") ) {
                if ( value.equalsIgnoreCase("1") ) {
                    state = 1;
                }
                if ( value.equalsIgnoreCase("0") ) {
                    state = 0;
                }
            }
            AM2H_Core::debugMessage("AM2H_Pcf8574::parseSetTopic()","key:value = "+key+":"+value, DebugLogger::INFO);
            key="";
            value="";
            continue;
        }
        if (isKey) { key+=c; } else { value+=c; }
    }

    if ( (port != 0xFF) && (state != 0xFF) ) {
        d.sensor.pcf8574.reg =  ( ( ~(1 << port) ) & d.sensor.pcf8574.reg ) | (state << port);
    }
}
