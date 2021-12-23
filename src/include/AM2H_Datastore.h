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

class Config {
    public:
    static constexpr uint16_t SET_0 = 0x0001;
    static constexpr uint16_t CHECK_TO_0 = 0x0001;
    static constexpr uint16_t SET_1 = 0x0002;
    static constexpr uint16_t CHECK_TO_1 = 0x0003;
    static constexpr uint16_t SET_2 = 0x0004;
    static constexpr uint16_t CHECK_TO_2 = 0x0007;
    static constexpr uint16_t SET_3 = 0x0008;
    static constexpr uint16_t CHECK_TO_3 = 0x000F;
    static constexpr uint16_t SET_4 = 0x0010;
    static constexpr uint16_t CHECK_TO_4 = 0x001F;
    static constexpr uint16_t SET_5 = 0x0020;
    static constexpr uint16_t CHECK_TO_5 = 0x003F;
    static constexpr uint16_t SET_6 = 0x0040;
    static constexpr uint16_t CHECK_TO_6 = 0x007F;
    static constexpr uint16_t SET_7 = 0x0080;
    static constexpr uint16_t CHECK_TO_7 = 0x00FF;
    static constexpr uint16_t SET_8 = 0x0100;
    static constexpr uint16_t CHECK_TO_8 = 0x01FF;
    static constexpr uint16_t SET_9 = 0x0200;
    static constexpr uint16_t CHECK_TO_9 = 0x03FF;
    static constexpr uint16_t SET_A = 0x0400;
    static constexpr uint16_t CHECK_TO_A = 0x07FF;    
    static constexpr uint16_t SET_B = 0x0800;
    static constexpr uint16_t CHECK_TO_B = 0x0FFF;
    static constexpr uint16_t SET_C = 0x1000;
    static constexpr uint16_t CHECK_TO_C = 0x1FFF;
    static constexpr uint16_t SET_D = 0x2000;
    static constexpr uint16_t CHECK_TO_D = 0x3FFF;
    static constexpr uint16_t SET_E = 0x4000;
    static constexpr uint16_t CHECK_TO_E = 0x7FFF;
    static constexpr uint16_t SET_F = 0x8000;
    static constexpr uint16_t CHECK_TO_F = 0xFFFF;
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
        // Unit tickUnit;
        // DeriveUnit deriveUnit;
        uint32_t absCnt;       // units
        sint16 unitsPerTick;   // units/1
        sint16 exponentPerTick; // 10^e
        uint32_t unitsPerMs;      // units/ms
        sint16 exponentPerMs; // 10^e
        sint16 zeroLimit;      // ms
        uint32_t millis;  // lastMillis ms
    } icounter;

    struct Bme680 
    {
        uint32_t addr;
        sint16 offsetTemp;
        sint16 offsetHumidity;
    } bme680;
    
    struct Bh1750 {
        uint32_t addr;
        uint8 sensitivityAdjust;  // 31..254 default:69 
    } bh1750;

};

class AM2H_Datastore {
public:
    static constexpr uint8_t LOC_MAX_LEN{32};
    bool initialized{false};
    uint16_t config{0};
    char loc[LOC_MAX_LEN];
    String plugin;
    Datastore sensor;
    AM2H_Plugin* self{nullptr};
};
 
#endif