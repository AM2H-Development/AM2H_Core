#include "AM2H_Pcf8574.h"
#include "AM2H_Core.h"
// #include "include/AM2H_Helper.h"
// #include <Wire.h>

static_assert(PCF8574_MIN_RATE_MS > PCF8574_WAIT_READ_MS, "PCF8574_MIN_RATE_MS must be bigger than PCF8574_WAIT_READ_MS");

extern AM2H_Core *am2h_core;

void AM2H_Pcf8574::loopPlugin(AM2H_Datastore &d, const uint8_t index)
{
}

void AM2H_Pcf8574::timerPublish(AM2H_Datastore &d, PubSubClient &mqttClient, const String topic, const uint8_t index)
{
    const auto &pcf8574 = d.sensor.pcf8574;
    String ioMask;
    AM2H_Helper::formatBinary8(ioMask, pcf8574.ioMask);
    String reg;
    AM2H_Helper::formatBinary8(reg, pcf8574.reg);

    String config = F("ioMask=") + ioMask + F("&reg=") + reg;
    AM2H_Core::debugMessage("Pcf8574::tp()", F("publish ") + topic + F("state=") + config, DebugLogger::INFO);
    mqttClient.publish((topic + "state").c_str(), config.c_str());
    am2h_core->loopMqtt();
}

void AM2H_Pcf8574::interruptPublish(AM2H_Datastore &d, PubSubClient &mqttClient, const String topic, const uint8_t index)
{
    AM2H_Core::debugMessage(F("Pcf8574::ip()"), F("calling tp()"), DebugLogger::INFO);
    delay(PCF8574_WAIT_READ_MS);
    readReg(d);
    timerPublish(d, mqttClient, topic, index);
    uint8_t state = d.sensor.pcf8574.reg;
    delay(PCF8574_MIN_RATE_MS - PCF8574_WAIT_READ_MS);
    readReg(d);
    if (state != d.sensor.pcf8574.reg)
    {
        timerPublish(d, mqttClient, topic, index);
    }
}

void AM2H_Pcf8574::config(AM2H_Datastore &d, const MqttTopic &t, const String p)
{
    const auto CALLER = F("Pcf8574::tp()");

    AM2H_Core::debugMessage(F("Pcf8574::cfg()"), "old config=" + String(d.config, BIN), DebugLogger::INFO);
    if (t.meas_.equalsIgnoreCase("addr"))
    {
        d.sensor.pcf8574.addr = AM2H_Helper::parse_hex<uint32_t>(p);
        AM2H_Core::debugMessage(CALLER, "set addr=0x" + String(d.sensor.pcf8574.addr, HEX), DebugLogger::INFO);
        d.config |= Config::SET_0;
    }
    if (t.meas_.equalsIgnoreCase("loc"))
    {
        d.config &= ~Config::SET_1;
        if (p.length() > 0)
        {
            AM2H_Helper::parse_location(d.loc, p);
            AM2H_Core::debugMessage(CALLER, "set loc=" + String(d.loc), DebugLogger::INFO);
            d.config |= Config::SET_1;
        }
    }
    if (t.meas_.equalsIgnoreCase("ioMask"))
    {
        d.sensor.pcf8574.ioMask = p.toInt();
        String ioMask;
        AM2H_Helper::formatBinary8(ioMask, d.sensor.pcf8574.ioMask);
        AM2H_Core::debugMessage(CALLER, "set ioMask=" + ioMask, DebugLogger::INFO);
        d.config |= Config::SET_2;
    }
    if (t.meas_.equalsIgnoreCase("reg"))
    { // must be native
        d.sensor.pcf8574.reg = p.toInt();
        String reg;
        AM2H_Helper::formatBinary8(reg, d.sensor.pcf8574.reg);
        AM2H_Core::debugMessage(CALLER, "set reg=" + reg, DebugLogger::INFO);
        d.config |= Config::SET_3;
    }
    if (d.config == Config::CHECK_TO_3)
    {
        d.self = this;
        if (!d.initialized)
        {
            d.initialized = true;
            updateReg(d);
        }
        AM2H_Core::debugMessage(CALLER, F("new config=") + String(d.config, BIN), DebugLogger::INFO);
    }
    else
    {
        d.self = nullptr;
    }

    if (d.initialized && (t.meas_ == "setState"))
    {
        AM2H_Core::debugMessage("AM2H_Pcf8574::config()", "setState", DebugLogger::INFO);
        processSetStateTopic(d, p);
        updateReg(d);
        timerPublish(d, am2h_core->getMqttClient(), am2h_core->getDataTopic(d.loc, getSrv(), String(t.id_)), 0);
        return;
    }
    if (d.initialized && (t.meas_.equalsIgnoreCase("setPort")))
    {
        processSetPortTopic(d, p);
        updateReg(d);
        String reg;
        AM2H_Helper::formatBinary8(reg, d.sensor.pcf8574.reg);
        AM2H_Core::debugMessage("AM2H_Pcf8574::config()", "setPort reg = " + reg, DebugLogger::INFO);
        timerPublish(d, am2h_core->getMqttClient(), am2h_core->getDataTopic(d.loc, getSrv(), String(t.id_)), 0);
    }
}

void AM2H_Pcf8574::updateReg(AM2H_Datastore &d)
{
    am2h_core->switchWire(d.sensor.pcf8574.addr);
    Wire.beginTransmission(static_cast<uint8_t>(d.sensor.pcf8574.addr));
    uint8_t out = d.sensor.pcf8574.reg | ~d.sensor.pcf8574.ioMask;
    AM2H_Core::debugMessage("AM2H_Pcf8574::updateReg()", "sending " + String(out), DebugLogger::INFO);
    Wire.write(d.sensor.pcf8574.reg | ~d.sensor.pcf8574.ioMask);
    if (Wire.endTransmission() != 0)
    {
        AM2H_Core::debugMessage("AM2H_Pcf8574::updateReg()", "error while transmitting", DebugLogger::ERROR);
        return;
    };
}

void AM2H_Pcf8574::readReg(AM2H_Datastore &d)
{
    am2h_core->switchWire(d.sensor.pcf8574.addr);
    uint8_t num_bytes{0};
    num_bytes = Wire.requestFrom(static_cast<uint8_t>(d.sensor.pcf8574.addr), (uint8_t)1);
    if (num_bytes != 1)
    {
        AM2H_Core::debugMessage("AM2H_Pcf8574::readReg", "error while receiving", DebugLogger::ERROR);
        return;
    }
    d.sensor.pcf8574.reg = Wire.read();
}

void AM2H_Pcf8574::processSetStateTopic(AM2H_Datastore &d, const String &p_origin)
{
    auto &pcf8574 = d.sensor.pcf8574;
    String p = p_origin + "&";
    String key{};
    String value{};
    bool isKey{true};

    for (auto c : p)
    {
        if (c == '=')
        {
            isKey = false;
            continue;
        }
        if (c == '&')
        {
            isKey = true;
            if (key.equalsIgnoreCase("ioMask"))
            {
                pcf8574.ioMask = value.toInt();
            }
            if (key.equalsIgnoreCase("reg"))
            {
                pcf8574.reg = value.toInt();
            }
            AM2H_Core::debugMessage("AM2H_Pcf8574::parseStateTopic()", "key:value = " + key + ":" + value, DebugLogger::INFO);
            key = "";
            value = "";
            continue;
        }
        if (isKey)
        {
            key += c;
        }
        else
        {
            value += c;
        }
    }
}

void AM2H_Pcf8574::processSetPortTopic(AM2H_Datastore &d, const String &p_origin)
{
    // auto& pcf8574=d.sensor.pcf8574;
    String p = p_origin + "&";
    String key{};
    String value{};
    bool isKey{true};
    uint8_t port{0xFF};
    uint8_t state{0xFF};

    for (auto c : p)
    {
        if (c == '=')
        {
            isKey = false;
            continue;
        }
        if (c == '&')
        {
            isKey = true;
            if (key.equalsIgnoreCase("port"))
            {
                const long t_port = value.toInt();
                if ((t_port >= 0) && (t_port <= 7))
                {
                    port = t_port;
                }
            }
            if (key.equalsIgnoreCase("state"))
            {
                if (value.equalsIgnoreCase("1"))
                {
                    state = 1;
                }
                if (value.equalsIgnoreCase("0"))
                {
                    state = 0;
                }
            }
            AM2H_Core::debugMessage("AM2H_Pcf8574::parseSetTopic()", "key:value = " + key + ":" + value, DebugLogger::INFO);
            key = "";
            value = "";
            continue;
        }
        if (isKey)
        {
            key += c;
        }
        else
        {
            value += c;
        }
    }

    if ((port != 0xFF) && (state != 0xFF))
    {
        d.sensor.pcf8574.reg = ((~(1 << port)) & d.sensor.pcf8574.reg) | (state << port);
    }
}
