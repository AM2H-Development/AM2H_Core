#include "AM2H_Ds18b20.h"

void AM2H_Ds18b20::setup(){}

void AM2H_Ds18b20::loop(){}

void AM2H_Ds18b20::config(AM2H_Datastore& d, const MqttTopic& t, const String p){
    constexpr int setupComplete=7;
    if (t.meas_ == "addr") {
        d.sensor.ds18b20.addr=p.toInt();
        d.config |=1;
    }
    if (t.meas_ == "loc") {
        for (int i=0; i < p.length(); ++i){
            d.loc[i]= p.charAt(i);
        }
        d.loc[p.length()]='\0';
        d.config |=2;
    }
    if (t.meas_ == "offsetTemp") {
        d.sensor.ds18b20.offsetTemp=p.toInt();
        d.config |=4;
    }
    if ( d.config == setupComplete ){
        d.active = true;
        d.plugin = plugin_;
    } else {
        d.active = false;
    }
}