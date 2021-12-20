#ifndef AM2H_Bme680_h
#define AM2H_Bme680_h
#include "AM2H_Plugin.h"
#include "include/AM2H_MqttTopic.h"
#include "include/AM2H_Datastore.h"
#include "libs/BSEC_Software_Library/src/bsec.h"

class AM2H_Bme680 : public AM2H_Plugin {
public:
    AM2H_Bme680(const char* plugin, const char* srv): AM2H_Plugin(plugin,srv){};
    void setupPlugin(int datastoreIndex) override;
    void loopPlugin(int datastoreIndex) override;
    void config(AM2H_Datastore& d, const MqttTopic& t, const String p) override;
    void timerPublish(AM2H_Datastore& d, PubSubClient& mqttClient, const String topic) override;
private:
    Bsec bme680;
    uint8_t bsecState[BSEC_MAX_STATE_BLOB_SIZE] {0};
    uint16_t stateUpdateCounter {0};
    float temperature;
    float humidity;


    constexpr static uint8_t bsec_config_iaq[] {
        #include "libs/BSEC_Software_Library/src/config/generic_33v_3s_4d/bsec_iaq.txt"
    };
};

#endif