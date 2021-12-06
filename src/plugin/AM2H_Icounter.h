#ifndef AM2H_Icounter_h
#define AM2H_Icounter_h
#include "AM2H_Plugin.h"
#include "include/AM2H_MqttTopic.h"
#include "include/AM2H_Datastore.h"

class AM2H_Icounter : public AM2H_Plugin {
public:
    AM2H_Icounter(const char* plugin, const char* srv): AM2H_Plugin(plugin,srv){};
    void setupPlugin(int datastoreIndex) override;
    void config(AM2H_Datastore& d, const MqttTopic& t, const String p);
    void timerPublish(AM2H_Datastore& d, PubSubClient& mqttClient, const String topic);
    void interruptPublish(AM2H_Datastore& d, PubSubClient& mqttClient, const String topic);    
};

#endif