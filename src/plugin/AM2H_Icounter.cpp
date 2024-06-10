#include "AM2H_Icounter.h"
#include "AM2H_Core.h"

extern unsigned long lastImpulseMillis_G;
extern AM2H_Core *am2h_core;

void AM2H_Icounter::timerPublish(AM2H_Datastore &d, PubSubClient &mqttClient, const String topic, const uint8_t index)
{
    AM2H_Core::debugMessage(F("Ico::tp"), F("pub ") + topic, DebugLogger::INFO);

    const uint32_t timeout = (static_cast<uint32_t>(d.sensor.icounter.zeroLimit) * 1000) + lastImpulseMillis_G;
    if (timeout < millis())
    {
        mqttClient.publish((topic + F("power")).c_str(), "0");
        am2h_core->loopMqtt();
        mqttClient.publish((topic + F("counter")).c_str(), String(calculateCounter(d)).c_str());
        am2h_core->loopMqtt();
    }
    mqttClient.publish((topic + F("last")).c_str(), String(calculateLast(d)).c_str());
    am2h_core->loopMqtt();
}

void AM2H_Icounter::interruptPublish(AM2H_Datastore &d, PubSubClient &mqttClient, const String topic, const uint8_t index)
{
    const uint32_t del = ICOUNTER_MIN_IMPULSE_TIME_MS - (millis() - lastImpulseMillis_G);
    AM2H_Core::debugMessage(F("Ico::ip"), F("delay=") + String(del) + F(" state=") + String(digitalRead(CORE_ISR_PIN)), DebugLogger::INFO);
    if (del <= ICOUNTER_MIN_IMPULSE_TIME_MS)
    {
        delay(del);
    }
    if (digitalRead(CORE_ISR_PIN) == 1)
    {
        AM2H_Core::debugMessage(F("Ico::ip"), F("skipped, ISR_PIN not high"), DebugLogger::ERROR);
        return;
    }

    ++d.sensor.icounter.absCnt;
    const uint32_t interval = lastImpulseMillis_G - d.sensor.icounter.millis;
    d.sensor.icounter.millis = lastImpulseMillis_G;

    d.sensor.icounter.lastTimestamps[d.sensor.icounter.lastTimestampsPointer++] = lastImpulseMillis_G;
    if (d.sensor.icounter.lastTimestampsPointer > COUNTER_MAX_BUFFER)
    {
        d.sensor.icounter.lastTimestampsPointer = 0;
    }

    AM2H_Core::debugMessage(F("Ico::ip"), F("interval=") + String(interval) + F(" pub ") + topic, DebugLogger::INFO);
    mqttClient.publish((topic + F("counter")).c_str(), String(calculateCounter(d)).c_str());
    am2h_core->loopMqtt();
    mqttClient.publish((topic + F("power")).c_str(), String(calculatePower(d, interval)).c_str());
    am2h_core->loopMqtt();
    mqttClient.publish(d.sensor.icounter.absCntTopic, String(d.sensor.icounter.absCnt).c_str(), RETAINED);
    am2h_core->loopMqtt();

    const uint32_t del2 = ICOUNTER_MIN_IMPULSE_RATE_MS - (millis() - lastImpulseMillis_G);
    if (del2 <= ICOUNTER_MIN_IMPULSE_RATE_MS)
    {
        delay(del2);
    }
}

double AM2H_Icounter::calculateLast(const AM2H_Datastore &d)
{
    uint32_t ticks{0};
    for (uint8_t i = 0; i < COUNTER_MAX_BUFFER; ++i)
    {
        if (d.sensor.icounter.lastTimestamps[i] > (millis() - COUNTER_LAST_DURATION))
        {
            ticks++;
        }
    }
    return ticks * d.sensor.icounter.unitsPerTick * pow(10., d.sensor.icounter.exponentPerTick);
}

double AM2H_Icounter::calculateCounter(const AM2H_Datastore &d)
{
    return d.sensor.icounter.absCnt * d.sensor.icounter.unitsPerTick * pow(10., d.sensor.icounter.exponentPerTick);
}

double AM2H_Icounter::calculatePower(const AM2H_Datastore &d, const uint32_t interval)
{
    const unsigned long timeout = (static_cast<unsigned long>(d.sensor.icounter.zeroLimit) * 1000);
    if (interval > timeout)
    {
        return 0.;
    }
    const double power = 1. / (static_cast<double>(interval) / 1000) * static_cast<double>(d.sensor.icounter.unitsPerMs) * pow(10., static_cast<double>(d.sensor.icounter.exponentPerMs));
    AM2H_Core::debugMessage(F("Ico::cp"), F("power=") + String(power), DebugLogger::INFO);
    return power;
}

void AM2H_Icounter::config(AM2H_Datastore &d, const MqttTopic &t, const String p)
{
    AM2H_Core::debugMessage(F("Ico::cfg"), F("old cfg=") + String(d.config, BIN) + " " + t.meas_ + " : " + p, DebugLogger::INFO);

    if (!d.initialized)
    {
        d.initialized = true;
        d.sensor.icounter.lastTimestampsPointer = 0;
        for (uint8_t i = 0; i < COUNTER_MAX_BUFFER; ++i)
        {
            d.sensor.icounter.lastTimestamps[i] = 0;
        }

        String tempTopic = am2h_core->getFullStorageTopic(String(t.id_), t.srv_, "absCnt");
        d.sensor.icounter.absCntTopic = new char[tempTopic.length()];
        size_t i = 0;
        for (auto c : tempTopic)
        {
            d.sensor.icounter.absCntTopic[i++] = c;
        }
        d.sensor.icounter.absCntTopic[i] = '\0';

        am2h_core->subscribe(d.sensor.icounter.absCntTopic);
    }

    if (t.meas_.equalsIgnoreCase("absCnt"))
    {
        d.sensor.icounter.absCnt = p.toInt();
        d.sensor.icounter.millis = (UINT32_MAX / 2);
        am2h_core->unsubscribe(d.sensor.icounter.absCntTopic);
        d.config |= Config::SET_0;
    }
    if (t.meas_.equalsIgnoreCase("unitsPerTick"))
    {
        d.sensor.icounter.unitsPerTick = p.toInt();
        d.config |= Config::SET_1;
    }
    if (t.meas_.equalsIgnoreCase("exponentPerTick"))
    {
        d.sensor.icounter.exponentPerTick = p.toInt();
        d.config |= Config::SET_2;
    }
    if (t.meas_.equalsIgnoreCase("unitsPerMs"))
    {
        d.sensor.icounter.unitsPerMs = p.toInt();
        d.config |= Config::SET_3;
    }
    if (t.meas_.equalsIgnoreCase("exponentPerMs"))
    {
        d.sensor.icounter.exponentPerMs = p.toInt();
        d.config |= Config::SET_4;
    }
    if (t.meas_.equalsIgnoreCase("zeroLimit"))
    {
        d.sensor.icounter.zeroLimit = p.toInt();
        d.config |= Config::SET_5;
    }
    if (t.meas_.equalsIgnoreCase("loc"))
    {
        d.config &= ~Config::SET_6;
        if (p.length() > 0)
        {
            AM2H_Helper::parse_location(d.loc, p);
            d.config |= Config::SET_6;
        }
    }
    if (d.config == Config::CHECK_TO_6)
    {
        // d.plugin = plugin_;
        d.self = this;
        pinMode(CORE_ISR_PIN, INPUT_PULLUP);
        AM2H_Core::debugMessage(F("Ico::cfg"), F("new config=") + String(d.config, BIN), DebugLogger::INFO);
    }
    else
    {
        d.self = nullptr;
        // d.active = false;
    }
}