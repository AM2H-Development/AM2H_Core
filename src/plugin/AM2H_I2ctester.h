#ifndef AM2H_I2ctester_h
#define AM2H_I2Ctester_h
#include "AM2H_Plugin.h"

#define TCAADDR 0x70

class AM2H_I2ctester : public AM2H_Plugin
{
public:
    AM2H_I2ctester(const char *plugin, const char *srv) : AM2H_Plugin(plugin, srv){};
    void config(AM2H_Datastore &d, const MqttTopic &t, const String p) override;
    void timerPublish(AM2H_Datastore &d, PubSubClient &mqttClient, const String topic, const uint8_t index) override;

    String getHtmlTabContent() override;

private:
    uint8_t slave_addr = 0x01;
    uint8_t mux_addr = 0x00;
    uint8_t mux_ch = 0x00;
    uint8_t data[128];
    uint8_t data_length = 0;
    String instream{};
    String output{""};

    String parseParams(const String id);
    uint8_t parseNumber();
    char pop(String &instream);
    void tcaselect(uint8_t i);
    void scan();
    void setSlaveAdress();
    void setMuxAdress();
    void setChAdress();
    bool parseData();
    void sendI2C(uint8_t *data, uint8_t len);
    void readI2C();
    String printHex(uint8_t const num);
};

#endif