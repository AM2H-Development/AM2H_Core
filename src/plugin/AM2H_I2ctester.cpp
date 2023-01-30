#include "AM2H_I2ctester.h"
#include "AM2H_Core.h"
#include "include/AM2H_Helper.h"

extern AM2H_Core* am2h_core;

void AM2H_I2ctester::timerPublish(AM2H_Datastore& d, PubSubClient& mqttClient, const String topic, const uint8_t index){
}

void AM2H_I2ctester::config(AM2H_Datastore& d, const MqttTopic& t, const String p){
}

String AM2H_I2ctester::getHtmlTabContent() {
    instream = parseParams("qry");
    output ="";

    uint8_t c = pop(instream);
    switch(c) {
        case 'z':
            scan();
            break;
        case 'a':
            setSlaveAdress();
            break;
        case 'm':
            setMuxAdress();
            break;
        case 'c':
            setChAdress();
            break;
        case 's':
            if ( parseData() ) sendI2C(data, data_length);
            break;
        case 'r':
            readI2C();
            break;
    }

    String header = "<b>I2C Debugger V1.0</b><br />=====================<br />";
    header += "Slave Address&nbsp;: " + printHex(slave_addr) + "<br />";
    header += "Mux Address&nbsp;&nbsp;&nbsp;: " + printHex(mux_addr) + "<br />";
    header += "Mux Channel&nbsp;&nbsp;&nbsp;: " + String(mux_ch,DEC) + "<br />";
    header += "---------------------<br />";
    header += "z = Scan I2C addresses<br />";
    header += "a = Set slave address as HEX [a address]<br />";
    header += "m = Set mux address as HEX [a address]<br />";
    header += "c = Set channel address as DEC [a address]<br />";
    header += "s = Send data as HEX [s 00 FF ..]<br />";
    header += "r = Read number of Bytes [r nbrBytes]<br /><br />";
    header += "<form action=\"#\" method=\"post\">";
    header += "<label for=\"qry\">&nbsp;Action</label>&nbsp;";
    header += "<input type=\"text\" id=\"qry\" name=\"qry\" value=\"" + parseParams("qry") + "\" maxlength=\"16\" autofocus>";
    header += "<input type=\"submit\" value=\"Submit\"></form><br />";

    return header + output;
}

String AM2H_I2ctester::parseParams(const String qry) {
  for (uint8_t i = 0; i < am2h_core->server_.args(); i++) {
    if (qry.equalsIgnoreCase(am2h_core->server_.argName(i))) {
        return am2h_core->server_.arg(i);
    }
  }
  return "";
}

char AM2H_I2ctester::pop(String & instream){
    char tmp = instream.charAt(0);
    instream.remove(0,1);
    return tmp;
}

void AM2H_I2ctester::tcaselect(uint8_t i) {
    if (i > 7) return;
    uint8_t mux = (mux_addr>0)?mux_addr:TCAADDR;
    Wire.beginTransmission(mux);
    Wire.write(1 << i);
    Wire.endTransmission();
    output += "Mux Addr: " + printHex(mux) + " Channel: ";
    output += i;
    output += "<br />";
}

void AM2H_I2ctester::scan() {
    for (uint8_t t = 0; t < 7; t++) {
        uint8_t mux = (mux_addr>0)?mux_addr:TCAADDR;
        tcaselect(t);

        for (uint8_t addr = 0; addr <= 127; addr++) {
            if (addr == mux) continue;
            Wire.beginTransmission(addr);
            if (Wire.endTransmission() == 0) {
                output += "Found I2C " + printHex(addr);
                output += " (";
                output += String(addr, DEC);
                output += ")<br />";
            }
        }
    }
    output += "Scan completed.<br />";
}

uint8_t AM2H_I2ctester::parseNumber(){
    uint8_t c;
    int tmp = 0;
    while (true) {
        c = pop(instream);
        if(isDigit(c)) {
            tmp = tmp * 10 + (c - '0');
        } else if (isControl(c)) {
            break;
        }
    }
    return tmp;
}

void AM2H_I2ctester::setSlaveAdress() {
    uint8_t num = AM2H_Helper::parse_hex<uint8_t>(instream);
    if (num > 0 && num < 128) {
        slave_addr = num;
        output += "Set slave address: " + printHex(slave_addr) + "<br />";
    } else {
        output += "<b>Slave address not set! Only values in the range 0x01 - 0x7F are allowed</b><br />";
    }
}

void AM2H_I2ctester::setMuxAdress() {
    uint8_t num = AM2H_Helper::parse_hex<uint8_t>(instream);
    if (num > 0 && num < 128) {
        mux_addr = num;
        output += "Set mux address: " + printHex(mux_addr) + "<br />";
    } else {
        output += "<b>Mux address not set! Only values in the range 0x01 - 0x7F are allowed</b><br />";
    }
}

void AM2H_I2ctester::setChAdress() {
    uint8_t num = AM2H_Helper::parse_hex<uint8_t>(instream);
    if (num >= 0 && num < 8) {
        mux_ch = num;
        output += "Set mux channel: " + String(mux_ch,DEC) + "<br />";
    } else {
        output += "<b>Mux channel not set! Only values in the range 0-7 are allowed</b><br />";
    }
}

bool AM2H_I2ctester::parseData() {
    uint8_t tmp_data[128]; // Data
    uint8_t tmp_data_length = 0; // Length
    uint8_t pos_data = 0; // data octet
    uint8_t lower = 0; // nibble to process
    uint8_t c; // character read from serial

    output += "Parsed data: ";
    while(true) {
        c = pop(instream);
        if (isControl(c)) {
            break;
        } else if (!isHexadecimalDigit(c)) {
            continue;
        }

        output += (char) c;

        if(isLowerCase(c)) {
            c -= 87;
        } else if(isUpperCase(c)) {
            c -= 55;
        } else if(isDigit(c)){
            c -= 48;
        }

        if(lower == 0) {
            // process first nibble
            c = c << 4;
            pos_data = c;
            lower = 1;
        } else {
            // process second nibble
            pos_data |= c;
            tmp_data[tmp_data_length++] = pos_data;
            lower = 0;
        }
    }

    output += "<br />";
    if (lower == 1) {
        output += "<b>Data incomplete or malformed! Nothing to process<b><br />";
        return false;
    }
    memcpy(data, tmp_data, tmp_data_length);
    data_length = tmp_data_length;
    return true;
}

void AM2H_I2ctester::sendI2C(uint8_t *data, uint8_t len) {
    output += "Sending data [ ";
    for ( uint8_t i = 0; i < len; i++ ) {
        output += printHex(data[i]) + " ";
    }
    output += "] to Slave Addr: " + printHex(slave_addr) + " ";

    if (mux_addr>0) {
        tcaselect(mux_ch);
    }

    Wire.beginTransmission(slave_addr);
    Wire.write(data, len);
    Wire.endTransmission();
}

void AM2H_I2ctester::readI2C() {
    uint8_t num = parseNumber();

    if (num > 128) {
        output += "<b>Nothing read! Only values in the range 1-127 are allowed</b><br />";
        return;
    }

    output += "Reading " + String(num) + " Byte(s) from " + printHex(slave_addr) + "<br />";

    if (mux_addr>0) {
        tcaselect(mux_ch);
    }

   Wire.requestFrom(slave_addr, num);
    uint8_t buf[num];

    while (Wire.available()) {
        Wire.readBytes(buf, num);
    }

    for (int i=0; i < num; i++) {
        output += "Register value: ";
        output += printHex(buf[i]) + "<br />";
    }
}

String AM2H_I2ctester::printHex(uint8_t const num){
    String out{"0x"};
    if (num<16) { out +="0"; }
    out += String(num, HEX);
    return out;
}