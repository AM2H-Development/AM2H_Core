#ifndef AM2H_Icounter_h
#define AM2H_Icounter_h
#include "AM2H_Plugin.h"
#include <AM2H_MqttTopic.h>
#include <AM2H_Datastore.h>

class AM2H_Icounter : public AM2H_Plugin
{
public:
    AM2H_Icounter(const char *plugin, const char *srv) : AM2H_Plugin(plugin, srv){};
    void config(AM2H_Datastore &d, const MqttTopic &t, const String p);
    void timerPublish(AM2H_Datastore &d, PubSubClient &mqttClient, const String topic, const uint8_t index);
    void interruptPublish(AM2H_Datastore &d, PubSubClient &mqttClient, const String topic, const uint8_t index);

private:
    double calculatePower(AM2H_Datastore &d, const uint32_t interval);
    double calculateCounter(AM2H_Datastore &d);
    double calculateLast(AM2H_Datastore &d);
};

#endif