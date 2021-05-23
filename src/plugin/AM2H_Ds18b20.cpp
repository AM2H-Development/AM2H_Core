#include "AM2H_Ds18b20.h"
#include "AM2H_Core.h"
#include "libs/OneWire/OneWire.h"

extern AM2H_Core* am2h_core;

void AM2H_Ds18b20::setupPlugin(int datastoreIndex){
  AM2H_Core::debugMessage("\nDs18b20::Setup\n");

  OneWire* owds = am2h_core->getOwds();

  byte i;
  byte addr[8];

  owds->reset();
  owds->reset_search();
  delay (250);

  while ( owds->search(addr) ) {
    delay (250);

    AM2H_Core::debugMessage("ID = ");
    for( i = 0; i < 8; i++) {
      if ( addr[i] <=0xF ) { AM2H_Core::debugMessage("0"); }

      AM2H_Core::debugMessage(String(addr[i], HEX));
    }

    if (OneWire::crc8(addr, 7) != addr[7]) {
        AM2H_Core::debugMessage(" CRC is not valid!");
        return;
    }

    switch (addr[0]) {
      case 0x10:
        AM2H_Core::debugMessage(" Chip = DS18S20");  // or old DS1820
        break;
      case 0x28:
        AM2H_Core::debugMessage(" Chip = DS18B20");
        break;
      case 0x22:
        AM2H_Core::debugMessage(" Chip = DS1822");
        break;
      default:
        AM2H_Core::debugMessage(" Device is not a DS18x20 family device.");
    }
    AM2H_Core::debugMessage("\n");
  }
}

void AM2H_Ds18b20::timerPublish(AM2H_Datastore& d, PubSubClient& mqttClient, const String topic){
  AM2H_Core::debugMessage("Ds18b20::Publish: ");
  byte i;
  byte present = 0;
  byte type_s = ( d.sensor.ds18b20.addr[0] == 0x10 ) ? 1 : 0;
  byte data[12];
  float celsius;
  OneWire* owds = am2h_core->getOwds();

  for( i = 0; i < 8; i++) {
    if ( d.sensor.ds18b20.addr[i] <=0xF ) { AM2H_Core::debugMessage("0"); }
    AM2H_Core::debugMessage(String(d.sensor.ds18b20.addr[i], HEX));
  }

  owds->reset();
  owds->select( d.sensor.ds18b20.addr );
  owds->write(0x44, 1);        // start conversion, with parasite power on at the end

  delay(1000);

  present = owds->reset();
  owds->select( d.sensor.ds18b20.addr );
  owds->write(0xBE);         // Read Scratchpad

  for ( i = 0; i < 9; i++) { // we need 9 bytes
    data[i] = owds->read();
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

  celsius = (float)raw / 16.0;
  celsius += ( (float) d.sensor.ds18b20.offsetTemp / 10.0);

  AM2H_Core::debugMessage("  Temperature = ");
  AM2H_Core::debugMessage(String(celsius)+"\n");

  mqttClient.publish( (topic + "temperature").c_str() , String( celsius ).c_str() );
}

void AM2H_Ds18b20::config(AM2H_Datastore& d, const MqttTopic& t, const String p){
    AM2H_Core::debugMessage("Ds18b20::Config:");
    AM2H_Core::debugMessage(p);
    AM2H_Core::debugMessage("\n");
    if (t.meas_ == "addr") {
      AM2H_Core::debugMessage("Addr = ");
      for (int i=0; i < 8; ++i){
        d.sensor.ds18b20.addr[i]=strtol(p.substring(i*2,(i*2)+2).c_str(), nullptr,16);
        if ( d.sensor.ds18b20.addr[i] <=0xF ) { AM2H_Core::debugMessage("0"); }
        AM2H_Core::debugMessage(String(d.sensor.ds18b20.addr[i],HEX));
      }
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
        d.sensor.ds18b20.offsetTemp=p.toInt();
        d.config |= Config::SET_2;
    }
    if ( d.config == Config::CHECK_TO_2 ){
        d.plugin = plugin_;
        d.self = this;
        AM2H_Core::debugMessage("Config finished\n");
    } else {
        d.self=nullptr;
    }
}