#include "AM2H_Rgbled.h"
#include "AM2H_Core.h"
// #include "include/AM2H_Helper.h"
// #include <Wire.h>

extern AM2H_Core* am2h_core;

void AM2H_Rgbled::loopPlugin(AM2H_Datastore& d, const uint8_t index){
    auto& rgbled=d.sensor.rgbled;
    if (!rgbled.active) {return;}
    if ( (rgbled.timestamp+rgbled.time[rgbled.state]) < millis() ) {
        rgbled.state = (rgbled.state==State::ON) ? State::OFF : State::ON;
        if (rgbled.time[rgbled.state]==0){return;}
        am2h_core->switchWire(d.sensor.rgbled.addr);
        Wire.beginTransmission(static_cast<uint8_t>(d.sensor.rgbled.addr));
        Wire.write(OUTPUT_REGISTER);
        Wire.write(rgbled.color[rgbled.state]);
        Wire.endTransmission();
        rgbled.timestamp=millis();
    }
}

void AM2H_Rgbled::timerPublish(AM2H_Datastore& d, PubSubClient& mqttClient, const String topic, const uint8_t index){
    const auto& rgbled=d.sensor.rgbled;
    if (rgbled.active){
        String config = "colorOn="+getColorName(rgbled.color[State::ON])+"&colorOff="+getColorName(rgbled.color[State::OFF])+"&timeOn="+String(rgbled.time[State::ON])+"&timeOff="+String(rgbled.time[State::OFF]);
        AM2H_Core::debugMessageNl("AM2H_Rgbled::timerPublish()","publishing to " + topic + "state: " + config, DebugLogger::INFO);
        mqttClient.publish( (topic + "state").c_str() , config.c_str() );
        am2h_core->loopMqtt();
    } else {
        mqttClient.publish( (topic + "state").c_str() , "active=FALSE" );
        am2h_core->loopMqtt();
    }
}

void AM2H_Rgbled::config(AM2H_Datastore& d, const MqttTopic& t, const String p){
    AM2H_Core::debugMessageNl("AM2H_Rgbled::config()","old config state {"+String(d.config,BIN)+"}", DebugLogger::INFO);
    if ( d.initialized && (t.meas_ == "setState") ) {
        AM2H_Core::debugMessage("AM2H_Rgbled::config()","set state", DebugLogger::INFO);
        parseStateTopic(d,p);
        timerPublish(d, am2h_core->getMqttClient(), am2h_core->getDataTopic(d.loc,getSrv(),String(t.id_)),0 );
        return;
    }
    if (t.meas_ == "addr") {
        d.sensor.bh1750.addr= AM2H_Helper::parse_hex<uint32_t>(p);
        AM2H_Core::debugMessage("AM2H_Rgbled::config()"," set addr = 0x"+String(d.sensor.bh1750.addr,HEX), DebugLogger::INFO);
        d.config |= Config::SET_0;
    }
    if (t.meas_ == "loc") {
        d.config &= ~Config::SET_1;
        if (p.length()>0) {
            AM2H_Helper::parse_location(d.loc,p);
            AM2H_Core::debugMessage("AM2H_Rgbled::config()"," set loc = "+String(d.loc), DebugLogger::INFO);
            d.config |= Config::SET_1;
        }
    }
    if (t.meas_ == "colorOn") {
        d.sensor.rgbled.color[State::ON]=static_cast<uint8_t>(getColor(p));
        AM2H_Core::debugMessage("AM2H_Rgbled::config()"," set colorOn = "+String(d.sensor.rgbled.color[State::ON]), DebugLogger::INFO);
        d.config |= Config::SET_2;
    }
    if (t.meas_ == "colorOff") {
        d.sensor.rgbled.color[State::OFF]=static_cast<uint8_t>(getColor(p));
        AM2H_Core::debugMessage("AM2H_Rgbled::config()"," set colorOff = "+String(d.sensor.rgbled.color[State::OFF]), DebugLogger::INFO);
        d.config |= Config::SET_3;
    }
    if (t.meas_ == "timeOn") {
        d.sensor.rgbled.time[State::ON]= p.toInt();
        AM2H_Core::debugMessage("AM2H_Rgbled::config()"," set timeOn = "+String(d.sensor.rgbled.time[State::ON]), DebugLogger::INFO);
        d.config |= Config::SET_4;
    }
    if (t.meas_ == "timeOff") {
        d.sensor.rgbled.time[State::OFF]= p.toInt();
        AM2H_Core::debugMessage("AM2H_Rgbled::config()"," set timeOff = "+String(d.sensor.rgbled.time[State::OFF]), DebugLogger::INFO);
        d.config |= Config::SET_5;
    }
    if (t.meas_ == "active") {
        d.sensor.rgbled.active=AM2H_Helper::stringToBool(p);
        AM2H_Core::debugMessage("AM2H_Rgbled::config()"," set active = "+String(d.sensor.rgbled.active ? "TRUE": "FALSE"), DebugLogger::INFO);
        d.initialized=false;
        d.config |= Config::SET_6;
    }
    if ( d.config == Config::CHECK_TO_6 ){
        d.self = this;
        if (!d.initialized){
            postConfig(d);
            d.initialized=true;
        }
        AM2H_Core::debugMessageNl("AM2H_Rgbled::config()","finished, new config state {"+String(d.config,BIN)+"}", DebugLogger::INFO);
    } else {
        d.self=nullptr;
    }
}

void AM2H_Rgbled::postConfig(AM2H_Datastore& d){
    am2h_core->switchWire(d.sensor.rgbled.addr);
    Wire.beginTransmission(static_cast<uint8_t>(d.sensor.rgbled.addr));      //Init PCA9536 set register as an output
    Wire.write(CFG_REGISTER);
    Wire.write(0x00);
    Wire.endTransmission();
    delay(100);
    Wire.beginTransmission(static_cast<uint8_t>(d.sensor.rgbled.addr));
    Wire.write(OUTPUT_REGISTER);
    Wire.write(static_cast<uint8_t>(Color::BLACK));
    Wire.endTransmission();

    d.sensor.rgbled.timestamp=millis();
    d.sensor.rgbled.state=State::OFF;
}

void AM2H_Rgbled::parseStateTopic(AM2H_Datastore& d,const String& p_origin){
    auto& rgbled=d.sensor.rgbled;
    String p=p_origin+"&";
    String key{};
    String value{};
    bool isKey{true};
    bool isActive{true};

    for (auto c: p){
        if (c=='='){
            isKey=false;
            continue;
        }
        if (c=='&'){
            isKey=true;
            if (key=="colorOn") { rgbled.color[State::ON] = static_cast<uint8_t>(getColor(value)); }
            if (key=="colorOff") { rgbled.color[State::OFF] = static_cast<uint8_t>(getColor(value)); }
            if (key=="timeOn") { rgbled.time[State::ON] = value.toInt(); }
            if (key=="timeOff") { rgbled.time[State::OFF] = value.toInt(); }
            if (key=="active") { rgbled.active = AM2H_Helper::stringToBool(value); }            
            AM2H_Core::debugMessage("AM2H_Rgbled::parseStateTopic()","key:value = "+key+":"+value, DebugLogger::INFO);
            key="";
            value="";
            continue;
        }
        if (isKey) { key+=c; } else { value+=c; }
    }
    rgbled.active= isActive ? true : rgbled.active;
}

const AM2H_Rgbled::Color AM2H_Rgbled::getColor(const String& color){
    String uColor = color;
    uColor.toUpperCase();
    if (uColor=="WHITE"){ return Color::WHITE; }
    if (uColor=="FUCHSIA"){ return Color::FUCHSIA; }
    if (uColor=="AQUA"){ return Color::AQUA; }
    if (uColor=="BLUE"){ return Color::BLUE; }
    if (uColor=="RED"){ return Color::RED; }
    if (uColor=="YELLOW"){ return Color::YELLOW; }
    if (uColor=="GREEN"){ return Color::GREEN; }
    return Color::BLACK;    
}

const String AM2H_Rgbled::getColorName(const uint8_t col){
    const Color c = static_cast<Color>(col);
    if (c==Color::WHITE){ return "WHITE"; }
    if (c==Color::FUCHSIA){ return "FUCHSIA"; }
    if (c==Color::AQUA){ return "AQUA"; }
    if (c==Color::BLUE){ return "BLUE"; }
    if (c==Color::RED){ return "RED"; }
    if (c==Color::YELLOW){ return "YELLOW"; }
    if (c==Color::GREEN){ return "GREEN"; }
    return "BLACK";
}
