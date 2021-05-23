#ifndef AM2H_Sht21_h
#define AM2H_Sht21_h
#include "AM2H_Plugin.h"
#include "include/AM2H_MqttTopic.h"
#include "include/AM2H_Datastore.h"
#include "libs/SHT21/SHT21.h"

class AM2H_Sht21 : public AM2H_Plugin {
public:
    AM2H_Sht21(const char* plugin, const char* srv): AM2H_Plugin(plugin,srv){};
    void setupPlugin(int datastoreIndex) override;
    void config(AM2H_Datastore& d, const MqttTopic& t, const String p) override;
    void timerPublish(AM2H_Datastore& d, PubSubClient& mqttClient, const String topic) override;
private:
    SHT21 sht21;
};

#endif