#ifndef AM2H_Bh1750_h
#define AM2H_Bh1750_h
#include "libs/BH1750/BH1750.h"
#include "AM2H_Plugin.h"
#include "include/AM2H_MqttTopic.h"
#include "include/AM2H_Datastore.h"

class AM2H_Bh1750 : public AM2H_Plugin {
public:
    AM2H_Bh1750(const char* plugin, const char* srv): AM2H_Plugin(plugin,srv), lightMeter(0x23){};
    void setupPlugin(int datastoreIndex) override;
    void config(AM2H_Datastore& d, const MqttTopic& t, const String p) override;
    void timerPublish(AM2H_Datastore& d, PubSubClient& mqttClient, const String topic) override;
private:
    void postConfig(AM2H_Datastore& d) override;
    BH1750 lightMeter;
};

#endif