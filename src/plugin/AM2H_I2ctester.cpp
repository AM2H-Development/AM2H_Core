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
            readAddress();
            break;
        //case 'd':
            //  readData();
            //  break;
        case 's':
            if ( readData() ) sendI2C(data, data_length);
            break;
        case 'r':
            readI2C();
            break;
    }

    String header = "Serial to I2C <br />=====================<br />";
    header += "Slave Address: " + String(slave_addr,HEX) + "<br />";

    header += "Data to send: ";
    for ( uint8_t i=0; i < data_length; i++ ) {
        if ( i % 8 == 0 && i != 0 ) {
            header += "<br />      ";
        }
        if(data[i] < 16) {
            header += '0';
        }
        header += String(data[i], HEX);
    }
    header += "<br />Number of Bytes to read: " + String(nbrBytes);
    header += "<br />---------------------<br />";
    header += "z = Scan I2C address<br />";
    header += "a = Set slave address as DEC [a address]<br />";
    //Serial.println("d = Set data to send as HEX [d 00 FF ..]");
    header += "s = Send data as HEX [s 00 FF ..]<br />";
    header += "r = Read number of Bytes [r nbrBytes]<br /><br />";

    header += "<br />";
    header += "<form action=\"#\" method=\"post\">";
    header += "<label for=\"qry\">&nbsp;Action</label>&nbsp;";
    header += "<input type=\"text\" id=\"qry\" name=\"qry\" value=\"" + parseParams("qry") + "\" maxlength=\"16\">";
    header += "<input type=\"submit\" value=\"Submit\"></form><br />";


    return header + output;

}

// Helpers
// // //

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
    Wire.beginTransmission(TCAADDR);
    Wire.write(1 << i);
    Wire.endTransmission();
    output += "Select Channel ";
    output += i;
    output += "<br />";
}

void AM2H_I2ctester::scan() {
    for (uint8_t t = 0; t < 7; t++) {
        tcaselect(t);
        output += "TCA Port #";
        output += t;
        output += "<br />";

        for (uint8_t addr = 0; addr <= 127; addr++) {
            if (addr == TCAADDR) continue;
            Wire.beginTransmission(addr);
            if (Wire.endTransmission() == 0) {
                output += "Found I2C 0x";
                output += String(addr, HEX);
                output += " : ";
                output += String(addr, DEC);
                output += "<br />";
            }
        }
    }
    output += "Scan completed.<br />";
}

void AM2H_I2ctester::readAddress() {
    uint8_t c;
    int tmp = 0;
    while (true) {
        c = pop(instream);
        if(isDigit(c)) {
            tmp = tmp * 10 + (c - '0');
            output += c;
        } else if (c == 27) {
            return;
        } else if (isControl(c)) {
            break;
        }
    }

    if (tmp > 0 && tmp < 128) {
        slave_addr = tmp;
        output += "Slave address: " + String(slave_addr,HEX) + "<br />";
    } else {
        output += "<b>Slave address not set! Only values in the range 1-127 are allowed</b><br />";
    }
}

bool AM2H_I2ctester::readData() {
    uint8_t tmp_data[128]; // Data
    uint8_t tmp_data_length = 0; // Length
    uint8_t pos_data = 0; // data octet
    uint8_t lower = 0; // nibble to process
    uint8_t c; // character read from serial

    output += "Parsed data: ";

    while(true) {
        c = pop(instream);
        if (c == 27) {
            return false;
        } else if (isControl(c)) {
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
    output += "Sending data '";
    for ( uint8_t i = 0; i < len; i++ ) {
        if (data[i] < 16) {
            output += "0";
        }
        output += String(data[i], HEX);
    }
    output += "' to 0x" + String(slave_addr, HEX);

    Wire.beginTransmission(slave_addr);
    Wire.write(data, len);
    Wire.endTransmission();
}

void AM2H_I2ctester::readI2C() {
    uint8_t c;
    int tmp = 0;

    while (true) {
        c = pop(instream);
        if(isDigit(c)) {
            tmp = tmp * 10 + (c - '0');
        } else if (c == 27) {
            return;
        } else if (isControl(c)) {
            break;
        }
    }

    if (tmp > 0 && tmp < 128) {
        nbrBytes = tmp;
    } else {
        output += "<b>Nothing read! Only values in the range 1-127 are allowed</b><br />";
        nbrBytes=0;
        return;
    }

    output += "Reading " + String(nbrBytes) + " Byte(s) from 0x";
    output += String(slave_addr, HEX) + "<br />";

    Wire.requestFrom(slave_addr, nbrBytes);
    uint8_t buf[nbrBytes];

    while (Wire.available()) {
        Wire.readBytes(buf, nbrBytes);
    }

    for (int i=0; i < nbrBytes; i++) {
        output += "Register value: "+ String(buf[i], HEX) + "<br />";
    }
}