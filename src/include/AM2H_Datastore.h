#ifndef AM2H_Datastore_h
#define AM2H_Datastore_h
#include "Arduino.h"

union Datastore {
    struct Ds18b20 {
        uint32_t addr;
        char loc[32];
        sint16 offsetTemp;
    } ds18b20;

    struct Sht21 {
        uint32_t addr;
        char loc[32];
        sint16 offsetTemp;
        sint16 offsetHumidity;
    } sht21;

};

#endif