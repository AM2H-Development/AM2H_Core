#include "AM2H_Ds18b20.h"
#include "AM2H_Core.h"
#include "include/AM2H_Helper.h"
#include "libs/OneWire/OneWire.h"


extern AM2H_Core* am2h_core;

void AM2H_Ds18b20::setupPlugin(){
  AM2H_Core::debugMessage("AM2H_Ds18b20::setupPlugin()","scanning for devices:\n");

  byte i;
  byte addr[8];
  ows.reset();
  ows.reset_search();
  delay (250);

  while ( ows.search(addr) ) {
    delay (250);

    AM2H_Core::debugMessage("AM2H_Ds18b20::setupPlugin()","ID = ");
    for( i = 0; i < 8; i++) {
      if ( addr[i] <=0xF ) { AM2H_Core::debugMessage("AM2H_Ds18b20::setupPlugin()","0"); }
      AM2H_Core::debugMessage("AM2H_Ds18b20::setupPlugin()",String(addr[i], HEX));
    }

    if (OneWire::crc8(addr, 7) != addr[7]) {
      AM2H_Core::debugMessage("AM2H_Ds18b20::setupPlugin()"," CRC is not valid!\n");
      return;
    }

    switch (addr[0]) {
      case 0x10:
        AM2H_Core::debugMessage("AM2H_Ds18b20::setupPlugin()"," Chip = DS18S20\n");  // or old DS1820
        break;
      case 0x28:
        AM2H_Core::debugMessage("AM2H_Ds18b20::setupPlugin()"," Chip = DS18B20\n");
        break;
      case 0x22:
        AM2H_Core::debugMessage("AM2H_Ds18b20::setupPlugin()"," Chip = DS1822\n");
        break;
      default:
        AM2H_Core::debugMessage("AM2H_Ds18b20::setupPlugin()"," Device is not a DS18x20 family device.\n");
    }
  }
}

void AM2H_Ds18b20::timerPublish(AM2H_Datastore& d, PubSubClient& mqttClient, const String topic){
  AM2H_Core::debugMessage("AM2H_Ds18b20::timerPublish()","publishing to " + topic + ": addr = ");

  byte i;
  byte type_s = ( d.sensor.ds18b20.addr[0] == 0x10 ) ? 1 : 0;
  byte data[12];
  float celsius;

  for( i = 0; i < 8; i++) {
    if ( d.sensor.ds18b20.addr[i] <=0xF ) { AM2H_Core::debugMessage("AM2H_Ds18b20::timerPublish()","0"); }
    AM2H_Core::debugMessage("AM2H_Ds18b20::timerPublish()",String(d.sensor.ds18b20.addr[i], HEX));
  }

  ows.reset();
  ows.select( d.sensor.ds18b20.addr );
  ows.write(0x44, 1);        // start conversion, with parasite power on at the end
  delay(1000);
  ows.reset();
  ows.select( d.sensor.ds18b20.addr );
  ows.write(0xBE);         // Read Scratchpad

  for ( i = 0; i < 9; i++) { // we need 9 bytes
    data[i] = ows.read();
  }

  int16_t raw = (data[1] << 8) | data[0];
  if (type_s) {
    raw = raw << 3; // 9 bit resolution default
    if (data[7] == 0x10) {
      raw = (raw & 0xFFF0) + 12 - data[6];
    }
  } else {
    byte cfg = (data[4] & 0x60);
    // at lower res, the low bits are undefined, so let's zero them
    if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
    else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
    //// default is 12 bit resolution, 750 ms conversion time
  }

  celsius = static_cast<float>(raw) / 16.0;
  celsius += static_cast<float>(d.sensor.ds18b20.offsetTemp) / 10.0;

  AM2H_Core::debugMessage("AM2H_Ds18b20::timerPublish()","  Temperature = " + String(celsius) );
  mqttClient.publish( (topic + "temperature").c_str() , String( celsius ).c_str() );
  am2h_core->loopMqtt();
}

void AM2H_Ds18b20::config(AM2H_Datastore& d, const MqttTopic& t, const String p){
  AM2H_Core::debugMessage("AM2H_Ds18b20::config()","old config state {"+String(d.config,BIN)+"}\n");

  if (t.meas_ == "addr") {
    AM2H_Core::debugMessage("AM2H_Ds18b20::config()","set addr = 0x");
    for (int i=0; i < 8; ++i){
      d.sensor.ds18b20.addr[i]=strtol(p.substring(i*2,(i*2)+2).c_str(), nullptr,16);
      if ( d.sensor.ds18b20.addr[i] <=0xF ) { AM2H_Core::debugMessage("0"); }
      AM2H_Core::debugMessage("AM2H_Ds18b20::config()",String(d.sensor.ds18b20.addr[i],HEX));
    }
    d.config |= Config::SET_0;
  }
  if (t.meas_ == "loc") {
    AM2H_Helper::parse_location(d.loc,p);
    AM2H_Core::debugMessage("AM2H_Ds18b20::config()","set loc = "+String(d.loc));
    d.config |= Config::SET_1;
  }
  if (t.meas_ == "offsetTemp") {
    d.sensor.ds18b20.offsetTemp=p.toInt();
    AM2H_Core::debugMessage("AM2H_Ds18b20::config()","set offsetTemp = "+String(d.sensor.ds18b20.offsetTemp));
    d.config |= Config::SET_2;
  }
  if ( d.config == Config::CHECK_TO_2 ){
      // d.plugin = plugin_;
      d.self = this;
      AM2H_Core::debugMessage("AM2H_Ds18b20::config()","finished, new config state {"+String(d.config,BIN)+"}");
  } else {
      d.self=nullptr;
  }
}