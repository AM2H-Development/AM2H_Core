#ifndef AM2H_Rgbled_h
#define AM2H_Rgbled_h
#include "libs/BH1750/BH1750.h"
#include "AM2H_Plugin.h"
#include "include/AM2H_MqttTopic.h"
#include "include/AM2H_Datastore.h"

// Colors



class AM2H_Rgbled : public AM2H_Plugin {
public:
    AM2H_Rgbled(const char* plugin, const char* srv): AM2H_Plugin(plugin,srv){};
    void loopPlugin(AM2H_Datastore& d, const uint8_t index) override;
    void config(AM2H_Datastore& d, const MqttTopic& t, const String p) override;
    void timerPublish(AM2H_Datastore& d, PubSubClient& mqttClient, const String topic, const uint8_t index);
private:
    enum class Color{
        WHITE=0x0,
        FUCHSIA=0x01,
        AQUA=0x02,
        BLUE=0x03,
        YELLOW=0x04,
        RED=0x05,
        GREEN=0x06,
        BLACK=0x07
    };
    static constexpr uint8_t CFG_REGISTER{0x03};
    static constexpr uint8_t OUTPUT_REGISTER{0x01};

    const Color getColor(const String& color);
    const String getColorName(const uint8_t C);
    void postConfig(AM2H_Datastore& d) override;
    struct State{
        static constexpr uint8_t ON{0x00};
        static constexpr uint8_t OFF{0x01};
    };
    void parseStateTopic(AM2H_Datastore& d,const String& p);
};

#endif