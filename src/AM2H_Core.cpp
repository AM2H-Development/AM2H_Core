#include "AM2H_Core.h"
#include "include/AM2H_Core_Constants.h"
#include "include/AM2H_MqttTopic.h"
#include "include/AM2H_Datastore.h"

#define _SERIALDEBUG_  // enable serial debugging

unsigned long lastImpulseMillis_G{0};
unsigned long impulsesTotal_G{0};
bool intAvailable_G{false}; // Interupt available?

AM2H_Datastore ds[20];


void impulseISR()                                    // Interupt service routine
{
  if ( millis() - lastImpulseMillis_G >= 50 )            // 50ms Entprellzeit
  {
    lastImpulseMillis_G = millis();                    // Zeit merken
    ++impulsesTotal_G;
    intAvailable_G = true;                            // Daten zum Senden vorhanden
  }
}

AM2H_Core* am2h_core{nullptr};

AM2H_Core::AM2H_Core(AM2H_Plugin** plugins, PubSubClient& mqttClient, ESP8266WebServer& server):plugins_(plugins),mqttClient_(mqttClient),server_(server){
  status_=""; // status container for debugging messages
  updateRequired_ = NO_UPDATE_REQUIRED; // semaphore for system updates
  connStatus_ = CONN_UNKNOWN;

  String("192.168.178.10").toCharArray(persistentSetupValues_.mqttServer,MQTT_SERVER_LEN);
  persistentSetupValues_.mqttPort = 1883;
  String("myDevice12").toCharArray(persistentSetupValues_.deviceId,DEVICE_ID_LEN);
  String("myHome").toCharArray(persistentSetupValues_.ns,NS_LEN);
  volatileSetupValues_.ssid=""; 
  volatileSetupValues_.pw="";
  volatileSetupValues_.sampleRate=0;
  
  timer_.espRestart=0;
  timer_.mqttReconnect=0;
  timer_.wlanReconnect=0;
  timer_.sendData=0;
  
  pinMode(CORE_STATUS_LED, OUTPUT);
  pinMode(CORE_ISR_PIN, INPUT_PULLUP);
  attachInterrupt(CORE_ISR_PIN, impulseISR, FALLING);
  am2h_core= this;

}

void AM2H_Core::setup(){
  #ifdef _SERIALDEBUG_
    Serial.begin(115200);
    for (int i = 10; i > 0; i--) {
      Serial.print("Starting up - please wait ");
      Serial.println(i);
      delay(1000);
    }
    Serial.println("OK...");
  #endif

  setupEEPROM();
  setupWlan();
  setupServer();
  setupMqtt();

  int i=0;
  while ( auto p = plugins_[i++] ){
    p->setup();
  }

}

void AM2H_Core::loop(){
  loopServer();
  loopMqtt();
  checkUpdateRequired();

  if (volatileSetupValues_.sampleRate>0){
    if ( millis() > timer_.sendData ){
      timer_.sendData = millis() + volatileSetupValues_.sampleRate*1000;
        for (int i=0; i < 20; ++i){
          if (auto p = ds[i].self){
            p->timerPublish( ds[i],mqttClient_, getDataTopic( ds[i].loc, ds[i].self->getSrv(), String(i) ));
          }
        }
    }
  }

  if (isIntAvailable()) {
    for (int i=0; i < 20; ++i){
      if (auto p = ds[i].self){
        p->interruptPublish( ds[i],mqttClient_, getDataTopic( ds[i].loc, ds[i].self->getSrv(), String(i) ));
      }
    }
    resetInt();
  }

  for (int i=0; i < 20; ++i){
    if (auto p = ds[i].self){
      p->loop(i);
    }
  }

}

void AM2H_Core::checkUpdateRequired() {
  if (updateRequired_ != NO_UPDATE_REQUIRED) {
    if (updateRequired_ & COMMIT_TO_EEPROM_REQUIRED) {
      debugMessage("COMMIT_TO_EEPROM_REQUIRED\n");
      updateRequired_ ^= COMMIT_TO_EEPROM_REQUIRED;
      writeEEPROM();
    }
    if (updateRequired_ & WLAN_RESET_REQUIRED) {
      debugMessage("WLAN_RESET_REQUIRED\n");
      timer_.wlanReconnect = millis() + 1000;
      updateRequired_ ^= WLAN_RESET_REQUIRED;
    }
    if (updateRequired_ & MQTT_RESET_REQUIRED && (connStatus_ >= WLAN_CONNECTED) ) {
      debugMessage("MQTT_RESET_REQUIRED\n");
      mqttClient_.disconnect();
      mqttClient_.setServer(getMQTTServer(), getMQTTPort());
      connStatus_ = WLAN_CONNECTED;
      updateRequired_ ^= MQTT_RESET_REQUIRED;
    }
    if (updateRequired_ & ESP_RESET_REQUIRED) {
      debugMessage("ESP_RESET_REQUIRED\n");
      timer_.espRestart = millis() + 2000;
      updateRequired_ ^= ESP_RESET_REQUIRED;
      connStatus_ = DEV_RESTART_PENDING;
      pinMode(CORE_STATUS_LED, INPUT_PULLUP);
    }
  }

  if ( (timer_.wlanReconnect > 0) && (millis() > timer_.wlanReconnect) ) {
    debugMessage("Proceed WLAN reconnect!\n");
    timer_.wlanReconnect = 0;
    debugMessage(getSSID() + " : " + getPW() );
    restartWlan(getSSID(), getPW());
  }

  if ( (timer_.espRestart > 0) && (millis() > timer_.espRestart) ) {
    ESP.restart();
  }
}

// ----------
// ---------- EEPROM Utility Functions ---------------------------------------------------------
// ---------------------------------------------------------------------------------------------
void AM2H_Core::setupEEPROM() {
  debugMessage("EEPROM-Data needs size: ");
  debugMessage(String(sizeof(PersistentSetupContainer)));
  debugMessage("\n");
  EEPROM.begin(sizeof(PersistentSetupContainer));

  if (EEPROM.percentUsed() >= 0) {
    EEPROM.get(0, persistentSetupValues_);
    debugMessage("EEPROM has data from a previous run. ");
    debugMessage(String(EEPROM.percentUsed()));
    debugMessage("% of ESP flash space currently used\n");
  } else {
    debugMessage("EEPROM size changed - EEPROM data zeroed - write default values\n");
    writeEEPROM();
  }
}

void AM2H_Core::writeEEPROM() {
  EEPROM.put(0, persistentSetupValues_);
  boolean ok = EEPROM.commit();
  debugMessage( (ok) ? "Commit OK\n" : "Commit failed\n");
  debugMessage(String(EEPROM.percentUsed()));
  debugMessage("% of ESP flash space currently used\n");
}

// ----------
// ---------- Debug Message Handler ------------------------------------------------------------
// ---------------------------------------------------------------------------------------------

void AM2H_Core::debugMessage(String message) {
  if (am2h_core->status_.length() > 2000) am2h_core->status_ = am2h_core->status_.substring(500);
  am2h_core->status_ += message;
#ifdef _SERIALDEBUG_
  Serial.print(message);
#endif
}

// ----------
// ---------- WLAN Utility Functions -----------------------------------------------------------
// ---------------------------------------------------------------------------------------------

void AM2H_Core::setupWlan() {
  WiFi.mode(WIFI_STA);
  WiFi.begin();
  WiFi.hostname(getDeviceId());
  connectWlan(WLAN_TIMEOUT);
}

void AM2H_Core::restartWlan(String ssid, String pw) {
  connStatus_ = CONN_UNKNOWN;
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pw);
  connectWlan(WLAN_TIMEOUT);
}

void AM2H_Core::connectWlan(int timeout) {
  bool onOff{0};
  debugMessage("WIFI - connecting:");
  
  timeout *= 2;
  while ( (WiFi.status() != WL_CONNECTED) && ( timeout != 0) ) {
    digitalWrite(CORE_STATUS_LED,onOff);
    onOff = !onOff;
    delay(500);
    timeout--;
    debugMessage(".");
  }
  digitalWrite(CORE_STATUS_LED,LOW);
  
  if (timeout == 0) {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(getDeviceId());
    debugMessage("\n No connection established ... switching to AP mode with SSID=");
    debugMessage(getDeviceId());
    debugMessage("\n");
    connStatus_ = WLAN_AP_PROV;
  } else {
    debugMessage("Connected to ");
    debugMessage(WiFi.SSID() );
    debugMessage(" IP address: ");
    debugMessage(WiFi.localIP().toString());
    debugMessage("\r\n");
    connStatus_ = WLAN_CONNECTED;
  }
}

// ----------
// ---------- Server Response Handler + Utility ------------------------------------------------
// ---------------------------------------------------------------------------------------------
void AM2H_Core::setupServer() {
  debugMessage("setting up server handlers...\n");

  server_.on("/", handleRoot);
  server_.on(HTTP_API_V1_SET, handleApiSetRequest);
  server_.on(HTTP_API_V1_GET, handleApiGetRequest);
  server_.on(HTTP_API_V1_RESTART, handleRestart);
  server_.on(HTTP_API_V1_STATUS, handleStatus);
  server_.onNotFound(handleNotFound);
  server_.begin();
}

inline void AM2H_Core::loopServer(){
  server_.handleClient();
}

void AM2H_Core::handleRoot() {
  String content = "INDEX\n\n";
  content += "\nMethod: ";
  content += (am2h_core->server_.method() == HTTP_GET) ? "GET" : "POST";
  content += "\nArguments: ";
  content += am2h_core->server_.args();
  content += "\n";
  for (uint8_t i = 0; i < am2h_core->server_.args(); i++) {
    content += " " + am2h_core->server_.argName(i) + ": " + am2h_core->server_.arg(i) + "\n";
  }
  content += "\n\nSTATUS-LOG !-->\n";
  content += am2h_core->status_;
  content += "<--!\n";

#ifdef _SERIALDEBUG_
  Serial.print(content);
#endif
  am2h_core->server_.send(200, ENCODING_PLAIN, content);
}

void AM2H_Core::handleRestart() {
  debugMessage("Restart-request received\n");
  String content = "{\"message\":\"restart in 2 s!\"}";
  am2h_core->updateRequired_ |= ESP_RESET_REQUIRED;
  am2h_core->server_.send(200, ENCODING_JSON, content);
}

void AM2H_Core::handleStatus() {
  String content = "{\"status\":\"";
  content += am2h_core->status_;
  content += "\"}";
  am2h_core->server_.send(200, ENCODING_JSON, content);
}

void AM2H_Core::handleApiGetRequest() {
/*
{
  "deviceId":"myDevice9.123456789.123456789.12",
  "ssid":"ssid56789.123456789.123456789.12", "pw":"...",
  "mqttServer":"server-mh.fritz.box.123456789.123456789.123456789.",
  "mqttPort":1883,
  "ns":"ns345678"
}
*/
  String content = "{\n\"deviceId\":\"" + am2h_core->getDeviceId() + "\",\n";
  content += "\"ssid\":\"" + am2h_core->getSSID() + "\",\n\"pw\":\"********\",\n";
  content += "\"mqttServer\":\"" + String(am2h_core->getMQTTServer()) + "\",\n";
  content += "\"mqttPort\":\"" + String(am2h_core->getMQTTPort()) + "\",\n";
  content += "\"ns\":\"" + am2h_core->getNamespace() + "\"\n}";
  am2h_core->server_.send(200, ENCODING_JSON, content);
}

void AM2H_Core::handleApiSetRequest(){
  String content = "API set\n\n";
  content += "URI: ";
  content += am2h_core->server_.uri();
  content += "\nMethod: ";
  content += (am2h_core->server_.method() == HTTP_GET) ? "GET" : "POST";
  content += "\nArguments: ";
  content += am2h_core->server_.args();
  content += "\n";

  for (uint8_t i = 0; i < am2h_core->server_.args(); i++) {
    auto a = am2h_core->server_.argName(i);
    auto v = am2h_core->server_.arg(i);
    content += " " + a + ": " + v + "\n";

    if ( a == "deviceId" || a == "deviceid" ){
      debugMessage( "am2h_core->setDeviceId(v);" );
      am2h_core->setDeviceId(v);
    }

    if ( a == "ssid" ){
      am2h_core->setSSID(v);
    }
    
    if ( a == "pw" ){
      am2h_core->setPW(v);
    }

    if ( a == "mqttServer" || a == "mqttserver" ){
      am2h_core->setMQTTServer(v);
    }      

    if ( a == "mqttPort" || a == "mqttport" ){
      am2h_core->setMQTTPort(v);
    }

    if ( a == "ns" ){
      am2h_core->setNamespace(v);
    }
  }
#ifdef _SERIALDEBUG_
  Serial.print(content);
#endif
  am2h_core->server_.send(200, ENCODING_PLAIN, content);
}

void AM2H_Core::handleNotFound() {
  String content = "File Not Found\n\n";
  content += "URI: ";
  content += am2h_core->server_.uri();
  content += "\nMethod: ";
  content += (am2h_core->server_.method() == HTTP_GET) ? "GET" : "POST";
  content += "\nArguments: ";
  content += am2h_core->server_.args();
  content += "\n";
  for (uint8_t i = 0; i < am2h_core->server_.args(); i++) {
    content += " " + am2h_core->server_.argName(i) + ": " + am2h_core->server_.arg(i) + "\n";
  }
#ifdef _SERIALDEBUG_
  Serial.print(content);
#endif
  am2h_core->server_.send(404, ENCODING_PLAIN, content);
}

// ----------
// ---------- MQTT Utility Functions -----------------------------------------------------------
// ---------------------------------------------------------------------------------------------
void AM2H_Core::setupMqtt() {
  debugMessage( "setupMqtt()::" );
  debugMessage( getMQTTServer() );
  debugMessage( ":" );
  debugMessage( String(getMQTTPort()) );

  // mqttClient_.setServer("server-akm.fritz.box", getMQTTPort());
  mqttClient_.setServer(getMQTTServer(), getMQTTPort());
  mqttClient_.setCallback(AM2H_Core::mqttCallback);
}

void AM2H_Core::loopMqtt() {
  if ( connStatus_ >= WLAN_CONNECTED ) {
    if ( !mqttClient_.connected() ) {
      mqttReconnect();
    } else {
      mqttClient_.loop();
    }
  }
}

void AM2H_Core::mqttCallback(char* topic, uint8_t* payload, unsigned int length) {
  debugMessage( "Message arrived [" );
  debugMessage( topic );
  debugMessage( "]\n" );

  MqttTopic tp = AM2H_Core::parseMqttTopic(topic);
  // Global:
  // home / dev / esp01 / deviceCfg / - / sampleRate 
  // Plugins:
  // home / dev / esp01 / ds18b20 / id / addr 
  // ns_  /loc_ / dev_  / srv_ or plugin_ / id_ / meas_

  String s;
  for (int i = 0; i < length; i++) {
    s += (char) payload[i];
  }

  if ( tp.srv_ == DEVICE_CFG_TOPIC  ){
    debugMessage( "config: " );
    debugMessage( s );
    debugMessage( " #\n" );
    if ( tp.meas_ == "sampleRate" ){
      am2h_core->volatileSetupValues_.sampleRate = s.toInt();
      am2h_core->mqttClient_.publish(getStatusTopic().c_str(), ONLINE_PROP_VAL, RETAINED);
    }
  } else {
      // send cfg message to Plugin
      int i=0;
      while ( auto p = am2h_core->plugins_[i++] ){
        if (p->getPlugin() == tp.srv_ ){
          if ( tp.id_<20 ) {
            p->config(ds[tp.id_],tp,s);
          }
        }
      }
  }
}

void AM2H_Core::mqttReconnect() {
  if (timer_.mqttReconnect < millis()) {
    debugMessage( "Attempting MQTT connection and unsubscribe topics ..." );
    mqttClient_.unsubscribe((getConfigTopic() + "#").c_str());

    if ( mqttClient_.connect(getDeviceId().c_str(), getStatusTopic().c_str(), 2, RETAINED, OFFLINE_PROP_VAL)) {
      debugMessage( "connected\n" );
      mqttClient_.publish(getStatusTopic().c_str(), CONFIG_PROP_VAL, RETAINED);
      debugMessage("\nPublish Status to \"config\": " );
      debugMessage( getStatusTopic() );
      debugMessage("\n");
      if ( !(updateRequired_ & MQTT_UPDATE_REQUIRED) ) {
        debugMessage("\nSubscribe Config: " );
        debugMessage( getConfigTopic() );
        debugMessage("\n" );
        mqttClient_.subscribe((getConfigTopic() + "#").c_str());
      }
      connStatus_ = MQTT_CLIENT_CONNECTED;
      // updateRequired ^= MQTT_RESET_REQUIRED;
    } else {
      debugMessage( "failed, rc=" );
      debugMessage( String(mqttClient_.state()) );
      debugMessage( " try again in 1 second\n" );
      timer_.mqttReconnect = millis() + 1000;
      connStatus_ = WLAN_CONNECTED;
    }
  }
}

void AM2H_Core::subscribe(const String loc, const String srv, const String id, const String meas){
  String topic = getNamespace() + "/" + loc + "/" + getDeviceId() + "/" + srv + "/" + id + "/" + meas;
  mqttClient_.subscribe( topic.c_str() );
}

String AM2H_Core::getStatusTopic() {
  return AM2H_Core::getConfigTopic() + STATUS_PROP_NAME;
}

String AM2H_Core::getConfigTopic() {
  return am2h_core->getNamespace() + "/" + DEVICE_PROP_NAME + "/" + am2h_core->getDeviceId() + "/";
}

String AM2H_Core::getDataTopic(const String loc, const String srv, const String id) {
  return am2h_core->getNamespace() + "/" + loc + "/" + am2h_core->getDeviceId() + "/" + srv + "/" + id + "/";
}

  bool AM2H_Core::isIntAvailable(){
    return intAvailable_G;
  }
  void AM2H_Core::resetInt(){
    intAvailable_G=false;
  }
  unsigned long AM2H_Core::getLastImpulseMillis(){
    return lastImpulseMillis_G;
  }

MqttTopic AM2H_Core::parseMqttTopic(char* topic){
  int i{0};
  String part[6];
  int p{0};
  // home/dev/esp01/devCfg/id/sampleRate\0

  while ( ( topic[i] != '\0' ) && ( p < 6 ))
  {
    if ( topic[i] != '/' ){
      part[p]+=topic[i];
    } else {
      ++p;
    }
    ++i;
  }

  return MqttTopic(part[0],part[1],part[2],part[3],part[4],part[5]);
}