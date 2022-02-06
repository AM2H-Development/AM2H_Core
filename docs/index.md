# Hardware documentation

AM2H modules are connected with standard 8 pin RJ45 network cables. Every AM2H ESP Wifi modul could be connected to selected sensors depending on the software configuration.

### RJ45 Pinout

| PIN | Description | Function |
| ----------- | ----------- | ----------- |
| 1 | GND | Power Supply
| 2 | VIN | Power Supply
| 3 | RX (3) | Onwire Data |
| 4 | TX (1) | Interrupt |
| 5 | GND | Power Supply |
| 6 | 3V3 | Power Supply |
| 7 | SCL (0) | I2C Serial Clock|
| 8 | SDA (2) | I2C Serial Data |

### Power supply Pin 1 and 2

AM2H modules are equipped with an AMS1117 3.3V Voltage Regulator. Limitations for the line regulation are within the range of 1.5V ≤ (VIN - VOUT) ≤ 12 V. The dropout voltage is around 1.5 Volt, **it is recommended to power the module with 5 Volt.** Power supply provides no additional protection circuit.

### DS18B20 1-Wire® Temperature Sensor Pin 3

DS18B20 is 1-Wire interface Temperature sensor manufactured by Dallas Semiconductor Corp. The unique 1-Wire® Interface requires only one digital pin for two way communication with a microcontroller. 
The sensor can be powered with a 3V to 5.5V power supply and consumes around 1mA during active temperature conversions. **An external 4.7k pull-up resistor between the signal and power supply is required to keep the data transfer stable.**

### Interrupt based pulse counter Pin 4

The module allows to counting pulses with an interrupt on Pin 4 which is set as internal pull-up. **An external pull-up resistor is not required.** The interrupt is set to trigger on the event of FALLING or LOW levels with a debounce time of 500 ms.

### Regulated Power Supply PIN 5 and 6

Output of the voltage regulator is 3.3 V. Depending on the input power supply the voltage regulator could deliver up to 800 mA of power. For further information please refer to the technical datasheet.

### I&#x00B2;C - Two Wire Interface Pin 7 and 8

I2C or I&#x00B2;C is short for Inter-Integrated Circuit, a synchronous serial communication protocol developed by Phillips for communication between a fast Microcontroller and relatively slow peripherals (like Memory or Sensors) using just two wires. It is also known as TWI (Two Wire Interface).
The I2C Bus consists of two wires called the Serial Data (SDA) and the Serial Clock (SCL). Data is transmitted through the SDA line while the SCL line is used to synchronize the devices with the clock signal. **An external 4.7k pull-up resistor for both bus lines (SDA and SCL) are required.**


### RJ45 Connector layout "bottom" (tap down)
![alt text](https://raw.githubusercontent.com/AM2H-Development/AM2H_Core/main/docs/assets/AM2H_RJ45_Pinout%20.png)

# Flashing the device

# AM2H_Core documentation

## REST API 
all HTTP/GET 

### configuration

```
  GET: api/v1/set?deviceId=myDevice9.123456789.123456789.12&ssid=ssid56789.123456789.123456789.12&pw=pw3456789.123456789.123456789.12&mqttServer=server-mh.fritz.box.123456789.123456789.123456789.&mqttPort=1883&ns=ns345678
  GET:  api/v1/restart  -> rebooting device
```
### information

```
  GET:  api/v1/get        -> {
                              "deviceId":"myDevice9.123456789.123456789.12",
                              "ssid":"ssid56789.123456789.123456789.12", "pw":"...",
                              "mqttServer":"server-mh.fritz.box.123456789.123456789.123456789.",
                              "mqttPort":1883,
                              "ns":"ns345678"
                            }
  GET:  api/v1/status   -> {"statuslog":"[...logger content...]"}
```
## MQTT configuration

### Core config

all retained

```
akm/config/d_100/-/core/sampleRate => 15
[akm/config/d_100/-/core/i2cMuxAddr => 0x70]
```

### Bme680 configuration

all retained

```
akm/config/d_100/0/Bme680/addr => 0x77 / 0x0277
akm/config/d_100/0/Bme680/loc => firstFloor
akm/config/d_100/0/Bme680/offsetTemp => 0 // in 0.1 °C
akm/config/d_100/0/Bme680/offsetHumidity => 0 // in 0.1 % rH
akm/config/d_100/0/Bme680/offsetPressure => 0 // in 0.1 hPa

akm/storage/d_100/0/Bme680/setIAQ => 0 // only first time
```

### Bh1750 / (BH1721) configuration

all retained

```
akm/config/d_100/1/Bh1750/addr => 0x23 / 0x0423
akm/config/d_100/1/Bh1750/loc => firstFloor
akm/config/d_100/1/Bh1750/sensitivityAdjust => 0 // for Bh1721 always 0, for Bh1750 31..254 - standard 69
```

### Rgbled configuration

all retained

```
akm/config/d_100/2/Rgbled/addr => 0x41 / 0x0341
akm/config/d_100/2/Rgbled/loc => firstFloor
akm/config/d_100/2/Rgbled/onTime => 1000 // in ms
akm/config/d_100/2/Rgbled/offTime => 1000 // in ms
akm/config/d_100/2/Rgbled/onColor => RED // one of [WHITE, FUCHSIA, AQUA, BLUE, YELLOW, RED, GREEN, BLACK] (case insensitive)
akm/config/d_100/2/Rgbled/offColor => RED // one of [WHITE, FUCHSIA, AQUA, BLUE, YELLOW, RED, GREEN, BLACK] (case insensitive)
akm/config/d_100/2/Rgbled/active => TRUE or >0 / FALSE or 0
akm/config/d_100/2/Rgbled/setState => ... // same as akm/firstFloor/signal/state + &active=TRUE/FALSE

```

### DS18B20 configuration

all retained

```
akm/config/d_100/3/Ds18b20/addr => 28ffd059051603cc
akm/config/d_100/3/Ds18b20/loc => firstFloor
akm/config/d_100/3/Ds18b20/offsetTemp => 0 // in 0.1 °C
```


