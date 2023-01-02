#include "AM2H_Pcf8574.h"
#include "AM2H_Core.h"
#include "include/AM2H_Helper.h"
#include <Wire.h>

extern AM2H_Core* am2h_core;

void AM2H_Pcf8574::loopPlugin(AM2H_Datastore& d, const uint8_t index){
}

void AM2H_Pcf8574::timerPublish(AM2H_Datastore& d, PubSubClient& mqttClient, const String topic, const uint8_t index){
    const auto& pcf8574=d.sensor.pcf8574;
    String config = "ioMask="+String(pcf8574.ioMask,BIN)+"&outNegate="+String(pcf8574.outNegate,BIN)+"&reg="+String(pcf8574.reg,BIN);
    AM2H_Core::debugMessageNl("AM2H_Pcf8574::timerPublish()","publishing to " + topic + "state: " + config, DebugLogger::INFO);
    mqttClient.publish( (topic + "state").c_str() , config.c_str() );
    am2h_core->loopMqtt();
}

void AM2H_Pcf8574::interruptPublish(AM2H_Datastore& d, PubSubClient& mqttClient, const String topic, const uint8_t index){
    AM2H_Core::debugMessageNl("AM2H_Pcf8574::interruptPublish()","calling timerPublish() ", DebugLogger::INFO);
    readReg(d);
    timerPublish(d,mqttClient,topic,index);
}

void AM2H_Pcf8574::config(AM2H_Datastore& d, const MqttTopic& t, const String p){
    AM2H_Core::debugMessageNl("AM2H_Pcf8574::config()","old config state {"+String(d.config,BIN)+"}", DebugLogger::INFO);
    if ( d.initialized && (t.meas_ == "setState") ) {
        AM2H_Core::debugMessage("AM2H_Pcf8574::config()","set state", DebugLogger::INFO);
        processStateTopic(d,p);
        timerPublish(d, am2h_core->getMqttClient(), am2h_core->getDataTopic(d.loc,getSrv(),String(t.id_)),0 );
        return;
    }
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
        AM2H_Core::debugMessage("AM2H_Pcf8574::config()"," set ioMask = "+String(d.sensor.pcf8574.ioMask,BIN), DebugLogger::INFO);
        d.config |= Config::SET_2;
    }
    if (t.meas_ == "outNegate") {
        d.sensor.pcf8574.outNegate=p.toInt();
        AM2H_Core::debugMessage("AM2H_Pcf8574::config()"," set outNegate = "+String(d.sensor.pcf8574.outNegate,BIN), DebugLogger::INFO);
        d.config |= Config::SET_3;
    }
    if (t.meas_ == "reg") {
        d.sensor.pcf8574.reg=p.toInt();
        AM2H_Core::debugMessage("AM2H_Pcf8574::config()"," set out = "+String(d.sensor.pcf8574.reg,BIN), DebugLogger::INFO);
        timerPublish(d, am2h_core->getMqttClient(), am2h_core->getDataTopic(d.loc,getSrv(),String(t.id_)),0 );
        d.config |= Config::SET_4;
    }
    if ( d.config == Config::CHECK_TO_4 ){
        d.self = this;
        if (!d.initialized){
            d.initialized=true;
        }
        AM2H_Core::debugMessageNl("AM2H_Pcf8574::config()","finished, new config state {"+String(d.config,BIN)+"}", DebugLogger::INFO);
    } else {
        d.self=nullptr;
    }

    if (t.meas_ == "set") {
        processSetTopic(d,p);
        timerPublish(d, am2h_core->getMqttClient(), am2h_core->getDataTopic(d.loc,getSrv(),String(t.id_)),0 );
        AM2H_Core::debugMessage("AM2H_Pcf8574::config()"," set reg = "+String(d.sensor.pcf8574.reg,BIN), DebugLogger::INFO);
    }
}

void AM2H_Pcf8574::updateReg(AM2H_Datastore& d){
    am2h_core->switchWire(d.sensor.pcf8574.addr);
    Wire.beginTransmission(static_cast<uint8_t>(d.sensor.pcf8574.addr));
    Wire.write(d.sensor.pcf8574.reg ^ d.sensor.pcf8574.outNegate);
    if ( Wire.endTransmission() != 0 ) {
        AM2H_Core::debugMessage("AM2H_Pcf8574::writeReg","error while transmitting", DebugLogger::ERROR);
        return;
    };
    delay(100);
    readReg(d);
}

void AM2H_Pcf8574::readReg(AM2H_Datastore& d){
    am2h_core->switchWire(d.sensor.pcf8574.addr);
    uint8_t num_bytes{0};
    num_bytes = Wire.requestFrom(static_cast<uint8_t>(d.sensor.pcf8574.addr),1);
    delay(100);
    if ( num_bytes != 1 ) {
        AM2H_Core::debugMessage("AM2H_Pcf8574::readReg","error while receiving", DebugLogger::ERROR);
        return;
    }
    d.sensor.pcf8574.reg = Wire.read();
}

void AM2H_Pcf8574::processStateTopic(AM2H_Datastore& d,const String& p_origin){
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
            if ( key.equalsIgnoreCase("outNegate") ) { pcf8574.outNegate = value.toInt(); }
            if ( key.equalsIgnoreCase("reg") ) { pcf8574.reg = value.toInt(); }
            AM2H_Core::debugMessage("AM2H_Pcf8574::parseStateTopic()","key:value = "+key+":"+value, DebugLogger::INFO);
            key="";
            value="";
            continue;
        }
        if (isKey) { key+=c; } else { value+=c; }
    }

    updateReg(d);
}

void AM2H_Pcf8574::processSetTopic(AM2H_Datastore& d,const String& p_origin){
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
                if ( (t_port >=0) && (t_port < 7) ) { port=t_port;}
            }
            if ( key.equalsIgnoreCase("state") ) {
                if ( value.equalsIgnoreCase("on") ) {
                    state = 1;
                }
                if ( value.equalsIgnoreCase("off") ) {
                    state =0;
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
        d.sensor.pcf8574.reg = ((0xFF ^ (1 << port)) & d.sensor.pcf8574.reg) & ((1 << port) & (0xFF*state)) & (0xFF ^ d.sensor.pcf8574.ioMask);
        updateReg(d);
    }
}
