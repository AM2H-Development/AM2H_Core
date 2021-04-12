#ifndef AM2H_Datastore_h
#define AM2H_Datastore_h
#include "Arduino.h"
#include <plugin/AM2H_Plugin.h>

class AM2H_Plugin;

union Datastore {
    struct Ds18b20 {
        uint32_t addr;
        sint16 offsetTemp;
    } ds18b20;

    struct Sht21 {
        uint32_t addr;
        sint16 offsetTemp;
        sint16 offsetHumidity;
    } sht21;

};

class AM2H_Datastore {
public:
    // bool active;
    sint16 config{0};
    char loc[32];
    String plugin;
    Datastore sensor;
    AM2H_Plugin* self{nullptr};
};
 
#endif