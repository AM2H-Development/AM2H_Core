#ifndef AM2H_Pcf8574_h
#define AM2H_Pcf8574_h
#include "AM2H_Plugin.h"
#include <AM2H_MqttTopic.h>
#include <AM2H_Datastore.h>

class AM2H_Pcf8574 : public AM2H_Plugin
{
public:
    AM2H_Pcf8574(const char *plugin, const char *srv) : AM2H_Plugin(plugin, srv){};
    void loopPlugin(AM2H_Datastore &d, const uint8_t index) override;
    void config(AM2H_Datastore &d, const MqttTopic &t, const String p) override;
    void timerPublish(AM2H_Datastore &d, PubSubClient &mqttClient, const String topic, const uint8_t index);
    void interruptPublish(AM2H_Datastore &d, PubSubClient &mqttClient, const String topic, const uint8_t index);

private:
    void updateReg(AM2H_Datastore &d);
    void readReg(AM2H_Datastore &d);
    void processSetStateTopic(AM2H_Datastore &d, const String &p);
    void processSetPortTopic(AM2H_Datastore &d, const String &p);
    void changedPortsPublish(AM2H_Datastore &d, PubSubClient &mqttClient, const String topic, const uint8_t &oldReg);
    void checkPublishers(AM2H_Datastore &d, PubSubClient &mqttClient, const u_int8_t port, const char edge);
};

#endif