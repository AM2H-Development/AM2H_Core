#ifndef AM2H_Pcf8574_h
#define AM2H_Pcf8574_h
#include "AM2H_Plugin.h"
#include "include/AM2H_MqttTopic.h"
#include "include/AM2H_Datastore.h"

// Colors



class AM2H_Pcf8574 : public AM2H_Plugin {
public:
    AM2H_Pcf8574(const char* plugin, const char* srv): AM2H_Plugin(plugin,srv){};
    void loopPlugin(AM2H_Datastore& d, const uint8_t index) override;
    void config(AM2H_Datastore& d, const MqttTopic& t, const String p) override;
    void timerPublish(AM2H_Datastore& d, PubSubClient& mqttClient, const String topic, const uint8_t index);
    void interruptPublish(AM2H_Datastore& d, PubSubClient& mqttClient, const String topic, const uint8_t index);

private:
    void updateReg(AM2H_Datastore& d);
    void readReg(AM2H_Datastore& d);
    void processStateTopic(AM2H_Datastore& d,const String& p);
    void processSetTopic(AM2H_Datastore& d,const String& p);
};

#endif