#ifndef AM2H_Bme680_h
#define AM2H_Bme680_h
#include "AM2H_Plugin.h"
#include <AM2H_MqttTopic.h>
#include <AM2H_Datastore.h>
#include <bsec.h>

class AM2H_Bme680 : public AM2H_Plugin
{
public:
    AM2H_Bme680(const char *plugin, const char *srv) : AM2H_Plugin(plugin, srv){};
    void loopPlugin(AM2H_Datastore &d, const uint8_t index) override;
    void config(AM2H_Datastore &d, const MqttTopic &t, const String p) override;
    void timerPublish(AM2H_Datastore &d, PubSubClient &mqttClient, const String topic, const uint8_t index) override;

private:
    void postConfig(AM2H_Datastore &d) override;
    void set_iaq(AM2H_Datastore &d, const String p);

    // Bsec bme680;
    // uint8_t bsecState[BSEC_MAX_STATE_BLOB_SIZE] {0};
    // uint16_t stateUpdateCounter {0};

    constexpr static uint8_t BSEC_CONFIG_IAQ[2048]{
        // BSEC_MAX_WORKBUFFER_SIZE
#include "../../lib/BSEC_Software_Library/src/config/generic_33v_3s_4d/bsec_iaq.txt"
    };

    constexpr static uint32_t STATE_SAVE_PERIOD{360 * 60 * 1000}; // 360 minutes - 4 times a day
    // uint16_t stateUpdateCounter {0};
};

#endif