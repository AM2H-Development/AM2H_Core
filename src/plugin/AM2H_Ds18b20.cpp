#include "AM2H_Ds18b20.h"
#include "AM2H_Core.h"

extern AM2H_Core *am2h_core;

void AM2H_Ds18b20::setupPlugin()
{
}

void AM2H_Ds18b20::timerPublish(AM2H_Datastore &d, PubSubClient &mqttClient, const String topic, const uint8_t index)
{
  const auto CALLER = F("Ds1::tp");
  String message = F("pub to ") + topic + F(" addr=");

  byte type_s = (d.sensor.ds18b20.addr[0] == 0x10) ? 1 : 0;
  byte data[12];
  float celsius;

  for (uint8_t i = 0; i < 8; i++)
  {
    if (d.sensor.ds18b20.addr[i] <= 0xF)
    {
      message += "0";
    }
    message += String(d.sensor.ds18b20.addr[i], HEX);
  }

  ows.reset();
  ows.select(d.sensor.ds18b20.addr);
  ows.write(0x44, 1); // start conversion, with parasite power on at the end
  am2h_core->wait(1000);
  ows.reset();
  ows.select(d.sensor.ds18b20.addr);
  ows.write(0xBE); // Read Scratchpad

  for (uint8_t i = 0; i < 9; i++)
  { // we need 9 bytes
    data[i] = ows.read();
  }

  int16_t raw = (data[1] << 8) | data[0];
  if (type_s)
  {
    raw = raw << 3; // 9 bit resolution default
    if (data[7] == 0x10)
    {
      raw = (raw & 0xFFF0) + 12 - data[6];
    }
  }
  else
  {
    byte cfg = (data[4] & 0x60);
    // at lower res, the low bits are undefined, so let's zero them
    if (cfg == 0x00)
      raw = raw & ~7; // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20)
      raw = raw & ~3; // 10 bit res, 187.5 ms
    else if (cfg == 0x40)
      raw = raw & ~1; // 11 bit res, 375 ms
    //// default is 12 bit resolution, 750 ms conversion time
  }

  celsius = static_cast<float>(raw) / 16.0;
  celsius += static_cast<float>(d.sensor.ds18b20.offsetTemp) / 10.0;

  message += F(" temp=");
  message += celsius;

  AM2H_Core::debugMessage(CALLER, message, DebugLogger::INFO);
  char error[] = "1";
  if (celsius <= 128. && celsius >= -55.)
  {
    error[0] = '0';
    mqttClient.publish((topic + F("temperature")).c_str(), String(celsius).c_str());
    am2h_core->loopMqtt();
  }
  else
  {
    String error_message{"Error "};
    for (int i = 0; i < 9; ++i)
    {
      String digits = String(data[i], HEX);
      if (digits.length() == 1)
      {
        digits = "0" + digits;
      }
      error_message += digits;
    }
    AM2H_Core::debugMessage(CALLER, error_message + "@" + String(index), DebugLogger::ERROR);
  }

  mqttClient.publish((topic + ERROR_CODE + "_" + String(index)).c_str(), error);
  am2h_core->loopMqtt();
}

void AM2H_Ds18b20::config(AM2H_Datastore &d, const MqttTopic &t, const String p)
{
  const auto CALLER = F("Ds1::cfg");
  AM2H_Core::debugMessage(CALLER, F("old cfg=") + String(d.config, BIN), DebugLogger::INFO);

  if (t.meas_.equalsIgnoreCase("addr"))
  {
    String message = F("set addr=0x");
    for (int i = 0; i < 8; ++i)
    {
      d.sensor.ds18b20.addr[i] = strtol(p.substring(i * 2, (i * 2) + 2).c_str(), nullptr, 16);
      if (d.sensor.ds18b20.addr[i] <= 0xF)
      {
        message += "0";
      }
      message += String(d.sensor.ds18b20.addr[i], HEX);
    }
    AM2H_Core::debugMessage(CALLER, message, DebugLogger::INFO);
    d.config |= Config::SET_0;
  }
  if (t.meas_.equalsIgnoreCase("loc"))
  {
    d.config &= ~Config::SET_1;
    if (p.length() > 0)
    {
      AM2H_Helper::parse_location(d.loc, p);
      AM2H_Core::debugMessage(CALLER, F("set loc=") + String(d.loc), DebugLogger::INFO);
      d.config |= Config::SET_1;
    }
  }
  if (t.meas_.equalsIgnoreCase("offsetTemp"))
  {
    d.sensor.ds18b20.offsetTemp = p.toInt();
    AM2H_Core::debugMessage(CALLER, F("set offsetTemp=") + String(d.sensor.ds18b20.offsetTemp), DebugLogger::INFO);
    d.config |= Config::SET_2;
  }
  if (d.config == Config::CHECK_TO_2)
  {
    // d.plugin = plugin_;
    d.self = this;
    AM2H_Core::debugMessage(CALLER, F("new config=") + String(d.config, BIN), DebugLogger::INFO);
  }
  else
  {
    d.self = nullptr;
  }
}

String AM2H_Ds18b20::getHtmlTabContent()
{
  AM2H_Core::debugMessage(F("Ds1::getHtml"), F("scanning DS18 devices:"), DebugLogger::INFO);
  String output{F("<b>18B20 Scanner V1.0</b><br />=====================<br />")};

  uint8_t i;
  uint8_t addr[8];
  uint8_t type_s{0};
  uint8_t data[12];

  ows.reset();
  ows.reset_search();
  am2h_core->wait(250);

  while (ows.search(addr))
  {
    output += "ID = ";
    for (i = 0; i < 8; i++)
    {
      if (addr[i] <= 0xF)
      {
        output += "0";
      }
      output += String(addr[i], HEX);
    }

    if (OneWire::crc8(addr, 7) != addr[7])
    {
      output += F(" CRC not valid!<br />");
      break;
    }

    ows.reset();
    ows.select(addr);
    ows.write(0x44, 1); // start conversion, with parasite power on at the end
    am2h_core->wait(1000);
    ows.reset();
    ows.select(addr);
    ows.write(0xBE); // Read Scratchpad

    for (i = 0; i < 9; i++)
    { // we need 9 bytes
      data[i] = ows.read();
    }

    int16_t raw = (data[1] << 8) | data[0];

    switch (addr[0])
    {
    case 0x10:
      output += F(" DS18S20");
      type_s = 1;
      break;
    case 0x28:
      output += F(" DS18B20");
      type_s = 0;
      break;
    case 0x22:
      output += F(" DS1822");
      type_s = 0;
      break;
    default:
      output += F(" not a DS18x20 device.<br />");
      type_s = 2;
    }

    if (type_s == 1)
    {
      raw = raw << 3; // 9 bit resolution default
      if (data[7] == 0x10)
      {
        raw = (raw & 0xFFF0) + 12 - data[6];
      }
    }
    else if (type_s == 0)
    {
      byte cfg = (data[4] & 0x60);
      // at lower res, the low bits are undefined, so let's zero them
      if (cfg == 0x00)
        raw = raw & ~7; // 9 bit resolution, 93.75 ms
      else if (cfg == 0x20)
        raw = raw & ~3; // 10 bit res, 187.5 ms
      else if (cfg == 0x40)
        raw = raw & ~1; // 11 bit res, 375 ms
      //// default is 12 bit resolution, 750 ms conversion time
    }
    else
    {
      break;
    }
    output += " Temp= " + String(static_cast<float>(raw) / 16.0) + "<br />";
  }
  return output;
}