#include "AM2H_I2ctester.h"
#include "AM2H_Core.h"

extern AM2H_Core *am2h_core;

void AM2H_I2ctester::timerPublish(AM2H_Datastore &d, PubSubClient &mqttClient, const String topic, const uint8_t index) {}
void AM2H_I2ctester::config(AM2H_Datastore &d, const MqttTopic &t, const String p) {}

String AM2H_I2ctester::getHtmlTabContent()
{
    instream = parseParams("qry");
    output = "";

    uint8_t c = pop(instream);
    switch (c)
    {
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
        if (parseData())
            sendI2C(data, data_length);
        break;
    case 'r':
        readI2C();
        break;
    }

    String header = F("<b>I2C Debugger V1.0</b><br />"
                      "=====================<br />"
                      "Slave Address&nbsp;: ") +
                    printHex(slave_addr);
    header += F("<br />Mux Addr&nbsp;&nbsp;&nbsp;: ") + printHex(mux_addr);
    header += F("<br />Mux Chan&nbsp;&nbsp;&nbsp;: ") + String(mux_ch, DEC);
    header += F("<br />---------------------<br />");
    header += F("z = Scan I2C bus<br />"
                "a = Set slave addr as HEX [a addr]<br />"
                "m = Set mux addr as HEX [m addr]<br />"
                "c = Set channel addr as DEC [c addr]<br />"
                "s = Send data as HEX [s 00 FF ..]<br />"
                "r = Read num of bytes [r nB]<br /><br />"
                "<form action=\"#\" method=\"post\">"
                "<label for=\"qry\">&nbsp;Action</label>&nbsp;"
                "<input type=\"text\" id=\"qry\" name=\"qry\" value=\"") +
              parseParams("qry");
    header += F("\" maxlength=\"16\" autofocus><input type=\"submit\" value=\"Submit\"></form><br />");

    return header + output;
}

String AM2H_I2ctester::parseParams(const String qry)
{
    for (uint8_t i = 0; i < am2h_core->server_.args(); i++)
    {
        if (qry.equalsIgnoreCase(am2h_core->server_.argName(i)))
        {
            return am2h_core->server_.arg(i);
        }
    }
    return "";
}

char AM2H_I2ctester::pop(String &instream)
{
    char tmp = instream.charAt(0);
    instream.remove(0, 1);
    return tmp;
}

void AM2H_I2ctester::tcaselect(uint8_t i)
{
    if (i > 7)
        return;
    uint8_t mux = (mux_addr > 0) ? mux_addr : TCAADDR;
    Wire.beginTransmission(mux);
    Wire.write(1 << i);
    Wire.endTransmission();
    output += F("Mux Addr: ") + printHex(mux) + F(" Chan: ");
    output += i;
    output += F("<br />");
}

void AM2H_I2ctester::scan()
{
    for (uint8_t t = 0; t < 8; t++)
    {
        uint8_t mux = (mux_addr > 0) ? mux_addr : TCAADDR;
        tcaselect(t);

        for (uint8_t addr = 0; addr <= 127; addr++)
        {
            if (addr == mux)
                continue;
            Wire.beginTransmission(addr);
            if (Wire.endTransmission() == 0)
            {
                output += F("Found ") + printHex(addr);
                output += " (";
                output += String(addr, DEC);
                output += F(")<br />");
            }
        }
    }
    output += F("done.<br />");
}

uint8_t AM2H_I2ctester::parseNumber()
{
    uint8_t c;
    int tmp = 0;
    while (true)
    {
        c = pop(instream);
        if (isDigit(c))
        {
            tmp = tmp * 10 + (c - '0');
        }
        else if (isControl(c))
        {
            break;
        }
    }
    return tmp;
}

void AM2H_I2ctester::setSlaveAdress()
{
    uint8_t num = AM2H_Helper::parse_hex<uint8_t>(instream);
    if (num > 0 && num < 128)
    {
        slave_addr = num;
        output += F("Set slave addr ") + printHex(slave_addr) + "<br />";
    }
    else
    {
        output += F("<b>Slave addr not set! Only vals in the range 0x01-0x7F valid</b><br />");
    }
}

void AM2H_I2ctester::setMuxAdress()
{
    uint8_t num = AM2H_Helper::parse_hex<uint8_t>(instream);
    if (num > 0 && num < 128)
    {
        mux_addr = num;
        output += F("Set mux addr ") + printHex(mux_addr) + "<br />";
    }
    else
    {
        output += F("<b>Mux addr not set! Only vals in the range 0x01-0x7F valid</b><br />");
    }
}

void AM2H_I2ctester::setChAdress()
{
    uint8_t num = AM2H_Helper::parse_hex<uint8_t>(instream);
    if (num >= 0 && num < 8)
    {
        mux_ch = num;
        output += F("Set mux chan ") + String(mux_ch, DEC) + "<br />";
    }
    else
    {
        output += F("<b>Mux chan not set! Only vals in the range 0-7 valid</b><br />");
    }
}

bool AM2H_I2ctester::parseData()
{
    uint8_t tmp_data[128];       // Data
    uint8_t tmp_data_length = 0; // Length
    uint8_t pos_data = 0;        // data octet
    uint8_t lower = 0;           // nibble to process
    uint8_t c;                   // character read from serial

    output += F("Parsed ");
    while (true)
    {
        c = pop(instream);
        if (isControl(c))
        {
            break;
        }
        else if (!isHexadecimalDigit(c))
        {
            continue;
        }

        output += (char)c;

        if (isLowerCase(c))
        {
            c -= 87;
        }
        else if (isUpperCase(c))
        {
            c -= 55;
        }
        else if (isDigit(c))
        {
            c -= 48;
        }

        if (lower == 0)
        {
            // process first nibble
            c = c << 4;
            pos_data = c;
            lower = 1;
        }
        else
        {
            // process second nibble
            pos_data |= c;
            tmp_data[tmp_data_length++] = pos_data;
            lower = 0;
        }
    }

    output += "<br />";
    if (lower == 1)
    {
        output += F("<b>Data incomplete or malformed!<b><br />");
        return false;
    }
    memcpy(data, tmp_data, tmp_data_length);
    data_length = tmp_data_length;
    return true;
}

void AM2H_I2ctester::sendI2C(uint8_t *data, uint8_t len)
{
    output += F("Sending [ ");
    for (uint8_t i = 0; i < len; i++)
    {
        output += printHex(data[i]) + " ";
    }
    output += F("] to slave addr ") + printHex(slave_addr) + " ";

    if (mux_addr > 0)
    {
        tcaselect(mux_ch);
    }

    Wire.beginTransmission(slave_addr);
    Wire.write(data, len);
    Wire.endTransmission();
}

void AM2H_I2ctester::readI2C()
{
    uint8_t num = parseNumber();

    if (num > 128)
    {
        output += F("<b>Nothing read! Only vals in the range 1-127 valid</b><br />");
        return;
    }

    output += F("Reading ") + String(num) + F(" byte(s) from ") + printHex(slave_addr) + "<br />";

    if (mux_addr > 0)
    {
        tcaselect(mux_ch);
    }

    Wire.requestFrom(slave_addr, num);
    uint8_t buf[num];

    while (Wire.available())
    {
        Wire.readBytes(buf, num);
    }

    for (int i = 0; i < num; i++)
    {
        output += F("Reg val ");
        output += printHex(buf[i]) + "<br />";
    }
}

String AM2H_I2ctester::printHex(uint8_t const num)
{
    String out{"0x"};
    if (num < 16)
    {
        out += "0";
    }
    out += String(num, HEX);
    return out;
}