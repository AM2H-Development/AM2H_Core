#include <AM2H_Core.h>
#include <plugin/AM2H_Plugin.h>
#include <plugin/AM2H_Ds18b20.h>
#include <plugin/AM2H_Icounter.h>

// AM2H ICounter
// Version 1.0.0 - 2020/11/26
//

/* API-Beschreibung:
    GET:  api/v1/get        -> {
                                "deviceId":"myDevice9.123456789.123456789.12",
                                "ssid":"ssid56789.123456789.123456789.12", "pw":"...",
                                "mqttServer":"server-mh.fritz.box.123456789.123456789.123456789.",
                                "mqttPort":1883,
                                "ns":"ns345678"
                             }

192.168.4.1
    GET: api/v1/set?deviceId=myDevice9.123456789.123456789.12&ssid=ssid56789.123456789.123456789.12&pw=pw3456789.123456789.123456789.12&mqttServer=server-mh.fritz.box.123456789.123456789.123456789.&mqttPort=1883&ns=ns345678

    GET:  api/v1/status   -> {"statuslog":"[...logger content...]"}

    GET:  api/v1/restart  -> rebooting device

*/

WiFiClient mqttWifiClient;
PubSubClient mqttClient(mqttWifiClient);
ESP8266WebServer server(80);

AM2H_Ds18b20 ds18b20("Ds18b20","envsense");
AM2H_Icounter icounter("Icounter","counter");


AM2H_Plugin* plugins[] {&ds18b20, &icounter, nullptr};

AM2H_Core core(plugins,mqttClient,server);

// ----------
// ---------- setup() --------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------


void setup()
{
  core.setup();
}

// ----------
// ---------- loop() ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------
void loop()
{
  core.loop();
}
