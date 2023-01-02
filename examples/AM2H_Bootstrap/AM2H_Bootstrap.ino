// AM2H Bootstrap
//

/* API-Beschreibung:
  Sample requests (default IP for AP-mode:192.168.4.1)

  GET:  api/v1/get        -> {
                              "deviceId":"myDevice9.123456789.123456789.12",
                              "ssid":"ssid56789.123456789.123456789.12", "pw":"...",
                              "mqttServer":"server-mh.fritz.box.123456789.123456789.123456789.",
                              "mqttPort":1883,
                              "ns":"ns345678"
                            }
  GET: api/v1/set?deviceId=myDevice9.123456789.123456789.12&ssid=ssid56789.123456789.123456789.12&pw=pw3456789.123456789.123456789.12&mqttServer=server-mh.fritz.box.123456789.123456789.123456789.&mqttPort=1883&ns=ns345678
  GET:  api/v1/status   -> {"statuslog":"[...logger content...]"}
  GET:  api/v1/restart  -> rebooting device

  MQTT Settings:
  home/dev/##esp01##/deviceCfg/-/sampleRate -> 15   (set sample rate to 15 s, if sampleRate==0 no TimerPublish is executed)

  For 18B20:
  home/dev/##esp01##/Ds18b20/##00..19##/addr -> e.g. 2895d54450013   (set OW-id of sensor)
  home/dev/##esp01##/Ds18b20/##00..19##/loc  -> e.g. livingRoom      (location of the sensor for mqtt publish)
  home/dev/##esp01##/Ds18b20/##00..19##/offsetTemp  -> e.g. -11      (offset temperature -1.1 °C)

  For Sht21:
  home/dev/##esp01##/Sht21/##00..19##/addr -> e.g. 0x40            (set I2C channel only mux addr will be parsed)
  home/dev/##esp01##/Sht21/##00..19##/loc  -> e.g. livingRoom      (location of the sensor for mqtt publish)
  home/dev/##esp01##/Sht21/##00..19##/offsetTemp  -> e.g. -11      (offset temperature -1.1 °C)
  home/dev/##esp01##/Sht21/##00..19##/offsetHumidity -> e.g. -11   (offset temperature -1.1 °C)

  For Bh1750:
  home/dev/##esp01##/Bh1750/##00..19##/addr -> e.g. 1..7            (set I2C channel)
  home/dev/##esp01##/Bh1750/##00..19##/loc  -> e.g. livingRoom      (location of the sensor for mqtt publish)
  home/dev/##esp01##/Bh1750/##00..19##/sensitivityAdjust  -> e.g. 69   (31..254)

  For Icounter:
  home/dev/##esp01##/Icounter/##00..19##/absCnt -> e.g. 112345
  home/dev/##esp01##/Icounter/##00..19##/unitsPerTick -> e.g. 1
  home/dev/##esp01##/Icounter/##00..19##/exponentPerTick  -> e.g. 0   (10^X)
  home/dev/##esp01##/Icounter/##00..19##/unitsPerMs -> e.g. 1
  home/dev/##esp01##/Icounter/##00..19##/exponentPerMs  -> e.g. 0   (10^X)
  home/dev/##esp01##/Icounter/##00..19##/zeroLimit  -> e.g. 100
  home/dev/##esp01##/Icounter/##00..19##/loc  -> e.g. gasMeter      (location of the sensor for mqtt publish)

*/
#include <AM2H_Core.h>
#include <plugin/AM2H_Ds18b20.h>
#include <plugin/AM2H_Sht21.h>
#include <plugin/AM2H_Bme680.h>
#include <plugin/AM2H_Bh1750.h>
#include <plugin/AM2H_Icounter.h>
#include <plugin/AM2H_Rgbled.h>
#include <plugin/AM2H_I2ctester.h>
#include <plugin/AM2H_Pcf8574.h>

AM2H_Ds18b20 ds18b20("Ds18b20","tempsense");
AM2H_Sht21 sht21("Sht21","envsense");
AM2H_Bme680 bme680("Bme680","envsense");
AM2H_Bh1750 bh1750("Bh1750","envsense");
AM2H_Icounter icounter("Icounter","countflowsense");
AM2H_Rgbled rgbled("Rgbled","signal");
AM2H_I2ctester i2ctester("I2ctester","debug");
AM2H_Pcf8574 pcf8574("Pcf8574","signal");

AM2H_Plugin* plugins[] {&ds18b20, &sht21, &bme680, &bh1750, &icounter, &rgbled, &i2ctester, &pcf8574, nullptr};
AM2H_Core core(plugins);

// ----------
// ---------- setup() --------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------
void setup()
{
  core.setupCore();
}

// ----------
// ---------- loop() ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------
void loop()
{
  core.loopCore();
}
