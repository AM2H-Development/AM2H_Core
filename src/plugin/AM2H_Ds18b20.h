#ifndef AM2H_Ds18b20_h
#define AM2H_Ds18b20_h
#include "libs/OneWire/OneWire.h"
#include "AM2H_Plugin.h"
#include "include/AM2H_MqttTopic.h"
#include "include/AM2H_Datastore.h"
#include "include/AM2H_Core_Constants.h"

class AM2H_Ds18b20 : public AM2H_Plugin {
public:
    AM2H_Ds18b20(const char* plugin, const char* srv): AM2H_Plugin(plugin,srv), ows(CORE_DQ) {};
    void setupPlugin() override;
    void config(AM2H_Datastore& d, const MqttTopic& t, const String p) override;
    void timerPublish(AM2H_Datastore& d, PubSubClient& mqttClient, const String topic) override;
protected:
    OneWire ows;
};

#endif