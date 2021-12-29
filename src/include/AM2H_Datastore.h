#ifndef AM2H_Datastore_h
#define AM2H_Datastore_h
#include "Arduino.h"
#include <plugin/AM2H_Plugin.h>
#include "bsec.h"

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

class Config {
    public:
    static constexpr uint16_t SET_0 = 0x0001;
    static constexpr uint16_t CHECK_TO_0 = SET_0;
    static constexpr uint16_t SET_1 = 0x0002;
    static constexpr uint16_t CHECK_TO_1 = CHECK_TO_0+SET_1;
    static constexpr uint16_t SET_2 = 0x0004;
    static constexpr uint16_t CHECK_TO_2 = CHECK_TO_1+SET_2;
    static constexpr uint16_t SET_3 = 0x0008;
    static constexpr uint16_t CHECK_TO_3 = CHECK_TO_2+SET_3;
    static constexpr uint16_t SET_4 = 0x0010;
    static constexpr uint16_t CHECK_TO_4 = CHECK_TO_3+SET_4;
    static constexpr uint16_t SET_5 = 0x0020;
    static constexpr uint16_t CHECK_TO_5 = CHECK_TO_4+SET_5;
    static constexpr uint16_t SET_6 = 0x0040;
    static constexpr uint16_t CHECK_TO_6 = CHECK_TO_5+SET_6;
    static constexpr uint16_t SET_7 = 0x0080;
    static constexpr uint16_t CHECK_TO_7 = CHECK_TO_6+SET_7;
    static constexpr uint16_t SET_8 = 0x0100;
    static constexpr uint16_t CHECK_TO_8 = CHECK_TO_7+SET_8;
    static constexpr uint16_t SET_9 = 0x0200;
    static constexpr uint16_t CHECK_TO_9 = CHECK_TO_8+SET_9;
    static constexpr uint16_t SET_A = 0x0400;
    static constexpr uint16_t CHECK_TO_A = CHECK_TO_9+SET_A;    
    static constexpr uint16_t SET_B = 0x0800;
    static constexpr uint16_t CHECK_TO_B = CHECK_TO_A+SET_B;
    static constexpr uint16_t SET_C = 0x1000;
    static constexpr uint16_t CHECK_TO_C = CHECK_TO_B+SET_C;
    static constexpr uint16_t SET_D = 0x2000;
    static constexpr uint16_t CHECK_TO_D = CHECK_TO_C+SET_D;
    static constexpr uint16_t SET_E = 0x4000;
    static constexpr uint16_t CHECK_TO_E = CHECK_TO_D+SET_E;
    static constexpr uint16_t SET_F = 0x8000;
    static constexpr uint16_t CHECK_TO_F = CHECK_TO_E+SET_F;
};

union Datastore {
    struct Ds18b20 {
        uint8_t addr[8];
        sint16 offsetTemp;
    } ds18b20;

    struct Sht21 {
        uint32_t addr;
        sint16 offsetTemp;
        sint16 offsetHumidity;
    } sht21;

    struct Icounter {
        uint32_t absCnt;       // units
        sint16 unitsPerTick;   // units/1
        sint16 exponentPerTick; // 10^e
        uint32_t unitsPerMs;      // units/ms
        sint16 exponentPerMs; // 10^e
        sint16 zeroLimit;      // ms
        uint32_t millis;  // lastMillis ms
    } icounter;

    struct Bme680 {
        Bsec* bme680;
        uint32_t addr;
        sint16 offsetTemp;
        sint16 offsetHumidity;
        sint16 offsetPressure;
        char*  iaqConfigTopic;
		uint8_t* iaqStateSave;
        bool iaqStateSetReady;
    } bme680;
    
    struct Bh1750 {
        uint32_t addr;
        uint8 sensitivityAdjust;  // 31..254 default:69 
    } bh1750;

    struct Rgbled {
        uint32_t addr;
        uint8_t color[2];
        uint16_t time[2];
        bool active;
        uint32_t timestamp;
        uint8_t state;
    } rgbled;
};

class AM2H_Datastore {
public:
    static constexpr uint8_t LOC_MAX_LEN{32};
    bool initialized{false}; // used to identify the first config run
    uint16_t config{0}; // array for "Config" state
    char loc[LOC_MAX_LEN];
    // String plugin; // name of the plugin, set after final config
    Datastore sensor;
    AM2H_Plugin* self{nullptr};
};
 
#endif