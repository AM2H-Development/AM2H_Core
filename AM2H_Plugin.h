#ifndef AM2H_Plugin_h
#define AM2H_Plugin_h

#include <AM2H_Core.h>

// MQTT Updates
#define VOL_ABS_UPDATE            0b00000001
#define VOL_FLOW_UPDATE           0b00000010

// Properties
#define   VOL_PER_TICK_PROP_VAL "volPerTick"
#define   MIN_FLOW_PROP_VAL "mFlow"
#define   VOL_ABS_PROP_VAL "volAbs"
#define   VOL_FLOW_PROP_VAL "volFlow"

class TestPlugin : public Plugin {
public:
    TestPlugin();
    void setup();
    void loop();
    void config(AM2H_Core& core, const char* key, const char* val);

private:
    void sendToMqtt(byte updateT);
    void checkNewMeasure();
    void calcMinFlowMillis();
    void calcPower();
    void checkZeroPower();
    String getVolumeAbsolutTopic();
    String getVolumeFlowTopic();
};

#endif