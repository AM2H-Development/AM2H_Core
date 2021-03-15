#ifndef AM2H_Ds18b20_h
#define AM2H_Ds18b20_h
#include "AM2H_Plugin.h"
#include "include/AM2H_MqttTopic.h"

class AM2H_Ds18b20 : public AM2H_Plugin {
public:
    AM2H_Ds18b20(const char* plugin): AM2H_Plugin(plugin){};
    void setup();
    void loop();
    void config(AM2H_Datastore& d, const MqttTopic t, const String p);
};

#endif