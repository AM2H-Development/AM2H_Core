#include "AM2H_Sht21.h"
#include "AM2H_Core.h"

extern AM2H_Core *am2h_core;

void AM2H_Sht21::timerPublish(AM2H_Datastore &d, PubSubClient &mqttClient, const String topic, const uint8_t index)
{
    const auto CALLER = F("Sht21::tp");
    am2h_core->switchWire(d.sensor.sht21.addr);
    float celsius = sht21.getTemperature();
    float humidity = sht21.getHumidity();
    char error[] = "1";
    if ((celsius != -46.85) && (humidity != -6.0))
    { // if sensor reading is valid calculate offsets
        AM2H_Core::debugMessage(CALLER, "pub " + topic + String(celsius) + " / " + String(humidity), DebugLogger::INFO);
        error[0] = '0';
        celsius += static_cast<float>(d.sensor.sht21.offsetTemp) / 10.0;
        humidity += static_cast<float>(d.sensor.sht21.offsetHumidity) / 10.0;
        mqttClient.publish((topic + "temperature").c_str(), String(celsius).c_str());
        am2h_core->loopMqtt();
        mqttClient.publish((topic + "humidity").c_str(), String(humidity).c_str());
        am2h_core->loopMqtt();
    }
    else
    {
        AM2H_Core::debugMessage(CALLER, String(index) + F(" Error ") + String(celsius) + "|" + String(humidity), DebugLogger::ERROR);
    }
    mqttClient.publish((topic + ERROR_CODE + "_" + String(index)).c_str(), error);
    am2h_core->loopMqtt();
}

void AM2H_Sht21::config(AM2H_Datastore &d, const MqttTopic &t, const String p)
{
    const auto CALLER = F("Sht21::cfg");
    AM2H_Core::debugMessage(CALLER, "old cfg=" + String(d.config, BIN), DebugLogger::INFO);

    if (t.meas_.equalsIgnoreCase("addr"))
    {
        d.sensor.sht21.addr = AM2H_Helper::parse_hex<uint32_t>(p);
        AM2H_Core::debugMessage(CALLER, F("set addr=0x") + String(d.sensor.sht21.addr, HEX), DebugLogger::INFO);
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
        d.sensor.sht21.offsetTemp = p.toInt();
        AM2H_Core::debugMessage(CALLER, F("set offsetTemp=") + String(d.sensor.sht21.offsetTemp), DebugLogger::INFO);
        d.config |= Config::SET_2;
    }
    if (t.meas_.equalsIgnoreCase("offsetHumidity"))
    {
        d.sensor.sht21.offsetHumidity = p.toInt();
        AM2H_Core::debugMessage(CALLER, F("set offsetHum=") + String(d.sensor.sht21.offsetHumidity), DebugLogger::INFO);
        d.config |= Config::SET_3;
    }
    if (d.config == Config::CHECK_TO_3)
    {
        d.self = this;
        AM2H_Core::debugMessage(CALLER, F("new cfg=") + String(d.config, BIN), DebugLogger::INFO);
    }
    else
    {
        d.self = nullptr;
    }
}