#ifndef AM2H_Datastore_h
#define AM2H_Datastore_h
#include "Arduino.h"

union AM2H_Datastore {
    bool active;
    sint16 config{0};
    char loc[32];
    String plugin;

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

#endif