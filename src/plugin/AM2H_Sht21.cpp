#include "AM2H_Sht21.h"
#include "AM2H_Core.h"
#include "include/AM2H_Helper.h"
#include "libs/SHT21/SHT21.h"

extern AM2H_Core* am2h_core;

void AM2H_Sht21::setupPlugin(){
  AM2H_Core::debugMessage("AM2H_Sht21::setupPlugin()","scanning for devices: ");
  scan();
}

void AM2H_Sht21::timerPublish(AM2H_Datastore& d, PubSubClient& mqttClient, const String topic){
    am2h_core->switchWire(d.sensor.sht21.addr);
    float celsius = sht21.getTemperature();
    float humidity = sht21.getHumidity();
    AM2H_Core::debugMessage("AM2H_Sht21::timerPublish()","publishing " + topic + String(celsius) + " / " + String (humidity));
    char error[] = "1";
    if ( (celsius != -46.85) && (humidity != -6.0 ) ) { // if sensor reading is valid calculate offsets
        error[0] = '0';
        celsius += static_cast<float>(d.sensor.sht21.offsetTemp) / 10.0;
        humidity += static_cast<float>(d.sensor.sht21.offsetHumidity) / 10.0;
        mqttClient.publish( (topic + "temperature").c_str() , String( celsius ).c_str() );
        am2h_core->loopMqtt();
        mqttClient.publish( (topic + "humidity").c_str() , String( humidity ).c_str() );
        am2h_core->loopMqtt();
    } else {
        i2cReset();
    }
    mqttClient.publish( (topic + ERROR_CODE).c_str() , error );
    am2h_core->loopMqtt();
}

void AM2H_Sht21::config(AM2H_Datastore& d, const MqttTopic& t, const String p){
    AM2H_Core::debugMessage("AM2H_Sht21::config()","old config state {"+String(d.config,BIN)+"}\n");

    if (t.meas_ == "addr") {
        d.sensor.sht21.addr=AM2H_Helper::parse_hex<uint32_t>(p);
        AM2H_Core::debugMessage("AM2H_Sht21::config()","set addr = 0x"+String(d.sensor.sht21.addr,HEX));
        d.config |= Config::SET_0;
    }
    if (t.meas_ == "loc") {
        d.config &= ~Config::SET_1;
        if (p.length()>0) {
            AM2H_Helper::parse_location(d.loc,p);
            AM2H_Core::debugMessage("AM2H_Sht21::config()","set loc = "+String(d.loc));
            d.config |= Config::SET_1;
        }
    }
    if (t.meas_ == "offsetTemp") {
        d.sensor.sht21.offsetTemp=p.toInt();
        AM2H_Core::debugMessage("AM2H_Sht21::config()","set offsetTemp = "+String(d.sensor.sht21.offsetTemp));
        d.config |= Config::SET_2;
    }
    if (t.meas_ == "offsetHumidity") {
        d.sensor.sht21.offsetHumidity=p.toInt();
        AM2H_Core::debugMessage("AM2H_Sht21::config()","set offsetHumidity = "+String(d.sensor.sht21.offsetHumidity));
        d.config |= Config::SET_3;
    }
    if ( d.config == Config::CHECK_TO_3 ){
        // d.plugin = plugin_;
        d.self = this;
        AM2H_Core::debugMessage("AM2H_Sht21::config()","finished, new config state {"+String(d.config,BIN)+"}");
    } else {
        d.self=nullptr;
    }
}

void AM2H_Sht21::tcaselect(uint8_t i){
    Wire.beginTransmission(TCAADDR);
    Wire.write(0);
    Wire.endTransmission();
    if (i > 7) return;
    Wire.beginTransmission(TCAADDR);
    Wire.write(1 << i);
    Wire.endTransmission();
};

void AM2H_Sht21::scan() {
    for (uint8_t t=0; t<8 ; ++t) {
        tcaselect(t);
        AM2H_Core::debugMessage("AM2H_Sht21::scan()", "TCA Port #" + String(t) + "\n");
        for (uint8_t addr = 0; addr <= 127; ++addr) {
            if (addr == TCAADDR) continue;
            Wire.beginTransmission(addr);
            int response = Wire.endTransmission();
            if (response == 0) {
                AM2H_Core::debugMessage("AM2H_Sht21::scan()","Found I2C 0x" + String(addr, HEX) + "\n");
            }
        }
    }
}

void AM2H_Sht21::i2cReset() {
    pinMode(CORE_SDA, INPUT_PULLUP); // Make SDA (data) and SCL (clock) pins Inputs with pullup.
    pinMode(CORE_SCL, INPUT_PULLUP);

    delay(500);
    boolean SCL_LOW;
    boolean SDA_LOW = (digitalRead(CORE_SDA) == LOW);  // vi. Check SDA input.
    int clockCount = 20; // > 2x9 clock

    while (SDA_LOW && (clockCount > 0)) { //  vii. If SDA is Low,
        clockCount--;
        // Note: I2C bus is open collector so do NOT drive SCL or SDA high.
        pinMode(CORE_SCL, INPUT); // release SCL pullup so that when made output it will be LOW
        pinMode(CORE_SCL, OUTPUT); // then clock SCL Low
        delayMicroseconds(10); //  for >5us
        pinMode(CORE_SCL, INPUT); // release SCL LOW
        pinMode(CORE_SCL, INPUT_PULLUP); // turn on pullup resistors again
        // do not force high as slave may be holding it low for clock stretching.
        delayMicroseconds(10); //  for >5us
        // The >5us is so that even the slowest I2C devices are handled.
        SCL_LOW = (digitalRead(CORE_SCL) == LOW); // Check if SCL is Low.
        int counter = 20;
        while (SCL_LOW && (counter > 0)) {  //  loop waiting for SCL to become High only wait 2sec.
            counter--;
            delay(100);
            SCL_LOW = (digitalRead(CORE_SCL) == LOW);
        }
        SDA_LOW = (digitalRead(CORE_SDA) == LOW); //   and check SDA input again and loop
    }

    // else pull SDA line low for Start or Repeated Start
    pinMode(CORE_SDA, INPUT); // remove pullup.
    pinMode(CORE_SDA, OUTPUT);  // and then make it LOW i.e. send an I2C Start or Repeated start control.
    // When there is only one I2C master a Start or Repeat Start has the same function as a Stop and clears the bus.
    /// A Repeat Start is a Start occurring after a Start with no intervening Stop.
    delayMicroseconds(10); // wait >5us
    pinMode(CORE_SDA, INPUT); // remove output low
    pinMode(CORE_SDA, INPUT_PULLUP); // and make SDA high i.e. send I2C STOP control.
    delayMicroseconds(10); // x. wait >5us
    pinMode(CORE_SDA, INPUT); // and reset pins as tri-state inputs which is the default state on reset
    pinMode(CORE_SCL, INPUT);
}
