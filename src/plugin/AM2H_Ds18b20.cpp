#include "AM2H_Ds18b20.h"
#include "AM2H_Core.h"
#include "libs/OneWire/OneWire.h"

// DS18B20: 28FFD059051603CC
// DS18B20: 28FFB09181150351
// DS18B20: 28FFC34001160448

extern AM2H_Core* am2h_core;

void AM2H_Ds18b20::setupPlugin(int datastoreIndex){
  AM2H_Core::debugMessage("Setup\n");

  OneWire* owds = am2h_core->getOwds();

  byte i;
  byte present = 0;
  byte type_s;
  byte data[12];
  byte addr[8];
  float celsius;

  if ( !(owds->search(addr)) ) {
    AM2H_Core::debugMessage("No more addresses.");
    owds->reset_search();
    delay(250);
    return;
  }

  AM2H_Core::debugMessage("ROM =");
  for( i = 0; i < 8; i++) {
    if ( addr[i] <=0xF ) { AM2H_Core::debugMessage("0"); }

    AM2H_Core::debugMessage(String(addr[i], HEX));
  }

  if (OneWire::crc8(addr, 7) != addr[7]) {
      AM2H_Core::debugMessage("CRC is not valid!");
      return;
  }

  // the first ROM byte indicates which chip
  switch (addr[0]) {
    case 0x10:
      AM2H_Core::debugMessage("  Chip = DS18S20");  // or old DS1820
      type_s = 1;
      break;
    case 0x28:
      AM2H_Core::debugMessage("  Chip = DS18B20");
      type_s = 0;
      break;
    case 0x22:
      AM2H_Core::debugMessage("  Chip = DS1822");
      type_s = 0;
      break;
    default:
      AM2H_Core::debugMessage("Device is not a DS18x20 family device.");
      return;
  }

  owds->reset();
  owds->select(addr);
  owds->write(0x44, 1);        // start conversion, with parasite power on at the end

  delay(1000);     // maybe 750ms is enough, maybe not
  // we might do a ds.depower() here, but the reset will take care of it.

  present = owds->reset();
  owds->select(addr);
  owds->write(0xBE);         // Read Scratchpad

  AM2H_Core::debugMessage("  Data = ");
  AM2H_Core::debugMessage(String(present, HEX));
  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = owds->read();
    AM2H_Core::debugMessage(String(data[i], HEX));
  }
  AM2H_Core::debugMessage(" CRC=");
  AM2H_Core::debugMessage(String(OneWire::crc8(data, 8), HEX));
  AM2H_Core::debugMessage("\n");

  // Convert the data to actual temperature
  // because the result is a 16 bit signed integer, it should
  // be stored to an "int16_t" type, which is always 16 bits
  // even when compiled on a 32 bit processor.
  int16_t raw = (data[1] << 8) | data[0];
  if (type_s) {
    raw = raw << 3; // 9 bit resolution default
    if (data[7] == 0x10) {
      // "count remain" gives full 12 bit resolution
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
  AM2H_Core::debugMessage("  Temperature = ");
  AM2H_Core::debugMessage(String(celsius)+"\n");
}

void AM2H_Ds18b20::timerPublish(AM2H_Datastore& d, PubSubClient& mqttClient, const String topic){
    AM2H_Core::debugMessage("Publish: ");
    AM2H_Core::debugMessage(topic+"\n");
    mqttClient.publish((topic + "test").c_str() ,"payload");
}

void AM2H_Ds18b20::config(AM2H_Datastore& d, const MqttTopic& t, const String p){
    AM2H_Core::debugMessage("Config:");
    AM2H_Core::debugMessage(p);
    AM2H_Core::debugMessage("\n");
    if (t.meas_ == "addr") {
      AM2H_Core::debugMessage("Addr= ");
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
        // d.active = true;
        d.plugin = plugin_;
        d.self = this;
        AM2H_Core::debugMessage("Config finished\n");
    } else {
        d.self=nullptr;
        // d.active = false;
    }
}