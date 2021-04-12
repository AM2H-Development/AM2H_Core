#ifndef AM2H_Datastore_h
#define AM2H_Datastore_h
#include "Arduino.h"
#include <plugin/AM2H_Plugin.h>

class AM2H_Plugin;

/*
enum Unit{
    QM=10,
    KWH,
    UNITS=500,
};

enum DeriveUnit{
    KW=10,
    LPERMIN,
    MPERS,
    UNITS=500
};
*/

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

    struct Icounter {
        // Unit tickUnit;
        // DeriveUnit deriveUnit;
        uint32_t absCnt;       // units
        sint16 unitsPerTick;   // units/1
        sint16 exponentPerTick; // 10^e
        uint32_t unitsPerMs;      // units/ms
        sint16 exponentPerMs; // 10^e
        sint16 zeroLimit;      // ms
    } icounter;
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