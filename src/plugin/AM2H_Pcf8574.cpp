#include "AM2H_Pcf8574.h"
#include "AM2H_Core.h"

static_assert(PCF8574_MIN_RATE_MS > PCF8574_WAIT_READ_MS, "PCF8574_MIN_RATE_MS must be bigger than PCF8574_WAIT_READ_MS");

extern AM2H_Core *am2h_core;

void AM2H_Pcf8574::loopPlugin(AM2H_Datastore &d, const uint8_t index)
{
}

void AM2H_Pcf8574::timerPublish(AM2H_Datastore &d, PubSubClient &mqttClient, const String topic, const uint8_t index)
{
    const auto &pcf8574 = d.sensor.pcf8574;

    delay(PCF8574_WAIT_READ_MS);
    readReg(d);

    String ioMask;
    AM2H_Helper::formatBinary8(ioMask, pcf8574.ioMask);
    String reg;
    AM2H_Helper::formatBinary8(reg, pcf8574.reg);

    String state = F("ioMask=") + ioMask + F("&reg=") + reg;
    AM2H_Core::debugMessage(F("Pcf::tp"), F("state=") + state + "@" + topic, DebugLogger::INFO);
    mqttClient.publish((topic + F("state")).c_str(), state.c_str());
    am2h_core->loopMqtt();
}

void AM2H_Pcf8574::interruptPublish(AM2H_Datastore &d, PubSubClient &mqttClient, const String topic, const uint8_t index)
{
    const auto &pcf8574 = d.sensor.pcf8574;

    AM2H_Core::debugMessage(F("Pcf::ip"), F("irq received"), DebugLogger::INFO);
    delay(PCF8574_WAIT_READ_MS);

    auto oldReg = pcf8574.reg;
    readReg(d);
    changedPortsPublish(d, mqttClient, topic, oldReg);
    timerPublish(d, mqttClient, topic, index);

    oldReg = d.sensor.pcf8574.reg;
    delay(PCF8574_MIN_RATE_MS - PCF8574_WAIT_READ_MS);
    readReg(d);
    if (oldReg != d.sensor.pcf8574.reg)
    {
        changedPortsPublish(d, mqttClient, topic, oldReg);
        timerPublish(d, mqttClient, topic, index);
    }
}

void AM2H_Pcf8574::config(AM2H_Datastore &d, const MqttTopic &t, const String p)
{
    const auto CALLER = F("Pcf::cfg");

    if (d.config == Config::IS_ZERO)
    {
        d.sensor.pcf8574.publisherCount = 0;
        d.config = Config::SET_0;
    }

    if (t.meas_.equalsIgnoreCase("addr"))
    {
        d.sensor.pcf8574.addr = AM2H_Helper::parse_number<uint32_t>(p);
        AM2H_Core::debugMessage(CALLER, "set addr=0x" + String(d.sensor.pcf8574.addr, HEX), DebugLogger::INFO);
        d.config |= Config::SET_1;
    }
    if (t.meas_.equalsIgnoreCase("loc"))
    {
        if (p.length() > 0)
        {
            AM2H_Helper::parse_location(d.loc, p);
            AM2H_Core::debugMessage(CALLER, F("set loc=") + String(d.loc), DebugLogger::INFO);
            d.config |= Config::SET_2;
        }
        else
        {
            d.config &= ~Config::SET_2; // RESET_1
        }
    }
    if (t.meas_.equalsIgnoreCase("ioMask"))
    {
        d.sensor.pcf8574.ioMask = AM2H_Helper::parse_number<uint8_t>(p);
        String ioMask;
        AM2H_Helper::formatBinary8(ioMask, d.sensor.pcf8574.ioMask);
        AM2H_Core::debugMessage(CALLER, "set ioMask=" + ioMask, DebugLogger::INFO);
        d.config |= Config::SET_3;
    }
    if (t.meas_.equalsIgnoreCase("reg"))
    { // must be native
        d.sensor.pcf8574.reg = AM2H_Helper::parse_number<uint8_t>(p);
        String reg;
        AM2H_Helper::formatBinary8(reg, d.sensor.pcf8574.reg);
        AM2H_Core::debugMessage(CALLER, "set reg=" + reg, DebugLogger::INFO);
        d.config |= Config::SET_4;
    }
    if (d.config == Config::CHECK_TO_4)
    {
        d.self = this;
        if (!d.initialized)
        {
            d.initialized = true;
            updateReg(d);
            AM2H_Core::debugMessage(CALLER, F("init ok=") + String(d.config, BIN), DebugLogger::INFO);
        }
    }
    else
    {
        d.self = nullptr;
    }

    if (d.initialized && (t.meas_.equalsIgnoreCase("setState")))
    {
        AM2H_Core::debugMessage(CALLER, "setState", DebugLogger::INFO);
        const auto oldReg = d.sensor.pcf8574.reg;
        processSetStateTopic(d, p);
        updateReg(d);
        readReg(d);
        auto &mqttClient = am2h_core->getMqttClient();
        const auto dataTopic = am2h_core->getDataTopic(d.loc, getSrv(), String(t.id_));
        changedPortsPublish(d, mqttClient, dataTopic, oldReg);
        timerPublish(d, mqttClient, dataTopic, t.id_);
        return;
    }
    if (d.initialized && (t.meas_.equalsIgnoreCase("setPort")))
    {
        const auto oldReg = d.sensor.pcf8574.reg;
        processSetPortTopic(d, p);
        updateReg(d);
        readReg(d);
        String reg;
        AM2H_Helper::formatBinary8(reg, d.sensor.pcf8574.reg);
        AM2H_Core::debugMessage(CALLER, F("setPort reg = ") + reg, DebugLogger::INFO);
        auto &mqttClient = am2h_core->getMqttClient();
        const auto dataTopic = am2h_core->getDataTopic(d.loc, getSrv(), String(t.id_));
        changedPortsPublish(d, mqttClient, dataTopic, oldReg);
        timerPublish(d, mqttClient, dataTopic, t.id_);
        return;
    }

    auto meas = t.meas_;
    meas.toLowerCase();
    if (meas.startsWith(F("onport")) && d.sensor.pcf8574.publisherCount < MAX_PUBLISHERS) // onPort1Rising | onPort1Falling
    {
        auto port = (uint8_t)meas.charAt(6) - 0x30;
        if (port < 8)
        {
            auto edge = meas.charAt(7);
            if (edge == 'f' || edge == 'r')
            {
                d.sensor.pcf8574.publisher[d.sensor.pcf8574.publisherCount] = new String{port};
                *d.sensor.pcf8574.publisher[d.sensor.pcf8574.publisherCount] += edge;
                *d.sensor.pcf8574.publisher[d.sensor.pcf8574.publisherCount] += p;
            }
            AM2H_Core::debugMessage(CALLER, "onPort" + *d.sensor.pcf8574.publisher[d.sensor.pcf8574.publisherCount], DebugLogger::INFO);
            ++d.sensor.pcf8574.publisherCount;
        }
    }
}

void AM2H_Pcf8574::updateReg(AM2H_Datastore &d)
{
    am2h_core->switchWire(d.sensor.pcf8574.addr);
    Wire.beginTransmission(static_cast<uint8_t>(d.sensor.pcf8574.addr));
    // xx00 0111
    // 1100 0000 ~0011 1111
    // 1100 0111
    uint8_t out = d.sensor.pcf8574.reg | ~d.sensor.pcf8574.ioMask;
    AM2H_Core::debugMessage(F("Pcf::updReg"), F("sending ") + String(out), DebugLogger::INFO);
    Wire.write(d.sensor.pcf8574.reg | ~d.sensor.pcf8574.ioMask);
    if (Wire.endTransmission() != 0)
    {
        AM2H_Core::debugMessage(F("Pcf::updReg"), F("error"), DebugLogger::ERROR);
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
        AM2H_Core::debugMessage(F("Pcf::rdReg"), F("error"), DebugLogger::ERROR);
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
            if (key.equalsIgnoreCase("reg"))
            {
                pcf8574.reg = AM2H_Helper::parse_number<uint8_t>(value) | ~d.sensor.pcf8574.ioMask;
            }
            AM2H_Core::debugMessage(F("Pcf::pst"), F("k:v=") + key + ":" + value, DebugLogger::INFO);
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
                const long t_port = AM2H_Helper::parse_number<uint8_t>(value);
                if ((t_port >= 0) && (t_port <= 7))
                {
                    port = t_port;
                }
            }
            if (key.equalsIgnoreCase("state"))
            {
                uint8_t state_tmp = AM2H_Helper::parse_number<uint8_t>(value);
                if (state_tmp == 1)
                {
                    state = 1;
                }
                if (state_tmp == 0)
                {
                    state = 0;
                }
            }
            AM2H_Core::debugMessage(F("Pcf::pspt"), F("k:v=") + key + ":" + value, DebugLogger::INFO);
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

void AM2H_Pcf8574::changedPortsPublish(AM2H_Datastore &d, PubSubClient &mqttClient, const String topic, const uint8_t &oldReg)
{
    const auto &pcf8574 = d.sensor.pcf8574;

    for (uint8_t i = 0; i < 8; ++i)
    {
        const auto bitSet = (pcf8574.reg & (1 << i)) > 0;
        const auto bitChanged = ((pcf8574.reg ^ oldReg) & (1 << i)) > 0;
        if (bitChanged)
        {
            AM2H_Core::debugMessage(F("Pcf::cpp"), topic + F("port") + i + F("=0b") + (bitSet ? "1" : "0"), DebugLogger::INFO);
            mqttClient.publish((topic + "port" + i).c_str(), bitSet ? "1" : "0");
            am2h_core->loopMqtt();
            checkPublishers(d, mqttClient, i, bitSet ? 'r' : 'f');
        }
    }
}

void AM2H_Pcf8574::checkPublishers(AM2H_Datastore &d, PubSubClient &mqttClient, const u_int8_t port, const char edge)
{
    for (u_int8_t i = 0; i < d.sensor.pcf8574.publisherCount; ++i)
    {
        auto pe = String{port} + edge;
        if (d.sensor.pcf8574.publisher[i]->startsWith(pe))
        {
            uint8_t pos{0};
            for (u_int32_t j = 0; j < d.sensor.pcf8574.publisher[i]->length(); ++j)
            {
                if (d.sensor.pcf8574.publisher[i]->charAt(j) == '>')
                {
                    pos = j;
                    break;
                }
            }
            if (pos == 0)
            {
                AM2H_Core::debugMessage(F("Pcf::cps"), d.sensor.pcf8574.publisher[i]->substring(2) + " no payload", DebugLogger::INFO);
                return;
            }
            auto message = d.sensor.pcf8574.publisher[i]->substring(2, pos);
            auto payload = d.sensor.pcf8574.publisher[i]->substring(pos + 1);
            AM2H_Core::debugMessage(F("Pcf::cps"), message + " > " + payload, DebugLogger::INFO);
            mqttClient.publish(message.c_str(), payload.c_str());
            am2h_core->loopMqtt();
        }
    }
}