#include "AM2H_Core.h"

// #define _SERIALDEBUG_OFF  // enable serial debugging

volatile unsigned long lastImpulseMillis_G{0};
volatile unsigned long impulsesTotal_G{0};
volatile bool intAvailable_G{false}; // Interupt available?

WiFiClient mqttWifiClient;
PubSubClient mqttClient(mqttWifiClient);
ESP8266WebServer server(80);

AM2H_Datastore ds[20];

void impulseISR(){                                    // Interupt service routine
  if ( millis() - lastImpulseMillis_G >= CORE_ISR_DEBOUNCE ){            // CORE_ISR_DEBOUNCE Entprellzeit
    lastImpulseMillis_G = millis();                    // Zeit merken
    intAvailable_G = true;                            // Daten zum Senden vorhanden
  }
}

AM2H_Core* am2h_core{nullptr};

AM2H_Core::AM2H_Core(AM2H_Plugin** plugins, PubSubClient& mqttClient, ESP8266WebServer& server):plugins_(plugins),mqttClient_(mqttClient),server_(server){
  mqttClient.setBufferSize(2500);
  Wire.begin(CORE_SDA,CORE_SCL);
  volatileSetupValues_.i2cMuxAddr=0;
  status_=""; // status container for debugging messages
  updateRequired_ = NO_UPDATE_REQUIRED; // semaphore for system updates
  connStatus_ = CONN_UNKNOWN;

  String( MQTT::DEFAULT_SERVER ).toCharArray(persistentSetupValues_.mqttServer,MQTT_SERVER_LEN);
  persistentSetupValues_.mqttPort = MQTT::DEFAULT_PORT;
  String( MQTT::DEVICE ).toCharArray(persistentSetupValues_.deviceId,DEVICE_ID_LEN);
  String( MQTT::NAMESPACE ).toCharArray(persistentSetupValues_.ns,NS_LEN);
  volatileSetupValues_.ssid=WiFi.SSID();
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

AM2H_Core::AM2H_Core(AM2H_Plugin** plugins):AM2H_Core(plugins, mqttClient, server){};

void AM2H_Core::setupCore(){
    Serial.begin(115200);
    debugMessage("AM2H_Core::setupCore()","AM2H_Core Version = " + VERSION + "\n" );
    debugMessage("AM2H_Core::setupCore()","starting up");
    Serial.print("AM2H_Core Version = " + VERSION + "\n");
    Serial.print("Starting up - please wait ");
    for (int i = 3; i > 0; --i) {
      debugMessage("AM2H_Core::setupCore()",".");
      Serial.print('.');
      delay(1000);
    }
    Serial.print("\nrunning!\n");
  #ifndef _SERIALDEBUG_
    Serial.flush();
    Serial.end();
  #endif

  setupEEPROM();
  setupWlan();
  setupServer();
  setupMqtt();

  int i=0;
  while ( auto p = plugins_[i++] ){
    debugMessage("AM2H_Core::setupCore()","Plugin-ID=" + String(i) + " : " );
    p->setupPlugin();
  }
}

void AM2H_Core::loopCore(){
  loopServer();
  loopMqtt();
  checkUpdateRequired();

  checkTimerPublish();
  checkIntPublish();
  loopPlugins();
}

void AM2H_Core::loopPlugins(){
  for (int datastoreIndex=0; datastoreIndex < 20; ++datastoreIndex){
    if (auto p = ds[datastoreIndex].self){
      p->loopPlugin(ds[datastoreIndex]);
    }
  }
}

void AM2H_Core::checkIntPublish(){
  if (isIntAvailable()) {
    intAvailable_G=false;
    ++impulsesTotal_G;
    for (int i=0; i < 20; ++i){
      if (auto p = ds[i].self){
        p->interruptPublish( ds[i],mqttClient_, getDataTopic( ds[i].loc, ds[i].self->getSrv(), String(i) ));
      }
    }
  }
}

void AM2H_Core::checkTimerPublish(){
  if (volatileSetupValues_.sampleRate>0){
    if ( millis() > timer_.sendData ){
      timer_.sendData = millis() + volatileSetupValues_.sampleRate*1000;
        debugMessage("AM2H_Core::checkTimerPublish()","");
        publishDeviceStatus();
        for (int i=0; i < 20; ++i){
          if (auto p = ds[i].self){
            p->timerPublish( ds[i], mqttClient_, getDataTopic( ds[i].loc, ds[i].self->getSrv(), String(i) ) );
            loopMqtt();
          }
        }
    }
  }
}

void AM2H_Core::checkUpdateRequired() {
  if (updateRequired_ != NO_UPDATE_REQUIRED) {
    if (updateRequired_ & COMMIT_TO_EEPROM_REQUIRED) {
      debugMessage("AM2H_Core::checkUpdateRequired()","COMMIT_TO_EEPROM_REQUIRED\n");
      updateRequired_ ^= COMMIT_TO_EEPROM_REQUIRED;
      writeEEPROM();
    }
    if (updateRequired_ & WLAN_RESET_REQUIRED) {
      debugMessage("AM2H_Core::checkUpdateRequired()","WLAN_RESET_REQUIRED\n");
      timer_.wlanReconnect = millis() + 1000;
      updateRequired_ ^= WLAN_RESET_REQUIRED;
    }
    if (updateRequired_ & MQTT_RESET_REQUIRED && (connStatus_ >= WLAN_CONNECTED) ) {
      debugMessage("AM2H_Core::checkUpdateRequired()","MQTT_RESET_REQUIRED\n");
      mqttClient_.disconnect();
      mqttClient_.setServer(getMQTTServer(), getMQTTPort());
      connStatus_ = WLAN_CONNECTED;
      updateRequired_ ^= MQTT_RESET_REQUIRED;
    }
    if (updateRequired_ & ESP_RESET_REQUIRED) {
      debugMessage("AM2H_Core::checkUpdateRequired()","ESP_RESET_REQUIRED\n");
      timer_.espRestart = millis() + 2000;
      updateRequired_ ^= ESP_RESET_REQUIRED;
      connStatus_ = DEV_RESTART_PENDING;
      pinMode(CORE_STATUS_LED, INPUT_PULLUP);
    }
  }

  if ( (timer_.wlanReconnect > 0) && (millis() > timer_.wlanReconnect) ) {
    debugMessage("AM2H_Core::checkUpdateRequired()","Proceed WLAN reconnect! " + getSSID() + " : " + getPW());
    timer_.wlanReconnect = 0;
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
  debugMessage("AM2H_Core::setupEEPROM()","EEPROM-Data size: "+String(sizeof(PersistentSetupContainer))+"\n");
  EEPROM.begin(sizeof(PersistentSetupContainer));

  if (EEPROM.percentUsed() >= 0) {
    EEPROM.get(0, persistentSetupValues_);
    debugMessage("AM2H_Core::setupEEPROM()","EEPROM has data from a previous run. " + String(EEPROM.percentUsed())+"% of ESP flash space currently used\n");
  } else {
    debugMessage("AM2H_Core::setupEEPROM()","EEPROM size changed - EEPROM data zeroed - write default values\n");
    writeEEPROM();
  }
  debugMessage("AM2H_Core::setupEEPROM()","ns: "+String(persistentSetupValues_.ns)+ ", deviceId:" + String(persistentSetupValues_.deviceId) + "\n");
}

void AM2H_Core::writeEEPROM() {
  EEPROM.put(0, persistentSetupValues_);
  boolean ok = EEPROM.commit();
  debugMessage("AM2H_Core::writeEEPROM()", (ok) ? "Commit to EEPROM OK\n" : "Commit to EEPROM failed\n");
}

// ----------
// ---------- Debug Message Handler ------------------------------------------------------------
// ---------------------------------------------------------------------------------------------

void const AM2H_Core::debugMessage(const String& caller, const String& message) {
  String newMessage;
  if ( am2h_core->lastCaller != caller ){
    newMessage+="\n["+String(millis()/1000.,3);
    newMessage+="] [H:"+String(ESP.getFreeHeap());
    newMessage+="] "+caller+" -> ";
    am2h_core->lastCaller=caller;
  }
  if (parse_debugMessage(message, newMessage)) {am2h_core->lastCaller="";}
  if (am2h_core->status_.length() > DebugLogger::LOGLEN) am2h_core->status_ = am2h_core->status_.substring(DebugLogger::SHORTBY);
  am2h_core->status_ += newMessage;
#ifdef _SERIALDEBUG_
  Serial.print(newMessage);
#endif
}
void const AM2H_Core::debugMessage(const String& message) {
  debugMessage(String(millis()), message);
}

bool const AM2H_Core::parse_debugMessage(const String message, String& newMessage) {
  bool nl{false};
  for (auto c: message){
    if (c=='\n'){
      newMessage+=" | ";
      nl=true;
      continue;}
    if (c<' ' || c>'~'){
      newMessage+="&#"+static_cast<uint8_t>(c)+';';
    } else {
      newMessage+=c;
    }
  }
  return nl;
}

// ----------
// ---------- WLAN Utility Functions -----------------------------------------------------------
// ---------------------------------------------------------------------------------------------

void AM2H_Core::setupWlan() {
  WiFi.mode(WIFI_STA);
  WiFi.persistent(false);
  WiFi.begin();
  WiFi.hostname(getDeviceId());
  connectWlan(WLAN_TIMEOUT);
}

void AM2H_Core::restartWlan(String ssid, String pw) {
  connStatus_ = CONN_UNKNOWN;
  WiFi.mode(WIFI_STA);
  WiFi.persistent(true);
  WiFi.begin(ssid, pw);
  connectWlan(WLAN_TIMEOUT);
}

void AM2H_Core::connectWlan(int timeout) {
  bool onOffLed{0};
  debugMessage("AM2H_Core::connectWlan()","WIFI - connecting to " + WiFi.SSID());

  timeout *= 2;
  while ( (WiFi.status() != WL_CONNECTED) && ( timeout != 0) ) {
    digitalWrite(CORE_STATUS_LED,onOffLed);
    onOffLed = !onOffLed;
    delay(500);
    timeout--;
    debugMessage("AM2H_Core::connectWlan()",".");
  }
  digitalWrite(CORE_STATUS_LED,LOW);

  if (timeout == 0) {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(getDeviceId());
    debugMessage("AM2H_Core::connectWlan()","\n No connection established ... switching to AP mode with SSID:"+getDeviceId()+"\n");
    connStatus_ = WLAN_AP_PROV;
  } else {
    debugMessage("AM2H_Core::connectWlan()","Connected to "+WiFi.SSID()+" IP address: "+WiFi.localIP().toString()+"\n");
    connStatus_ = WLAN_CONNECTED;
  }
}

// ----------
// ---------- Server Response Handler + Utility ------------------------------------------------
// ---------------------------------------------------------------------------------------------
void AM2H_Core::setupServer() {
  debugMessage("AM2H_Core::setupServer()","setting up server handlers\n");
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
  debugMessage("AM2H_Core::handleRoot()", String((am2h_core->server_.method() == HTTP_GET) ? "GET" : "POST") + " INDEX");
  for (uint8_t i = 0; i < am2h_core->server_.args(); i++) {
    debugMessage("AM2H_Core::handleRoot()/args", am2h_core->server_.argName(i) + ": " + am2h_core->server_.arg(i) + "\n");
  }
  am2h_core->server_.send(200, ENCODING_HTML, getRootContent());
}

const String AM2H_Core::getRootContent(){
  String content;
  content += HTTP_HEADER;
  content += "<h1>AM2H_Core</h1><p>device:" + String(am2h_core->persistentSetupValues_.deviceId) + "<br>fwVersion:" + String(VERSION) + "</p><ul>";
  content += "<li><a href=\"/api/v1/status\">show status logs</a></li>";
  content += "<li><a href=\"/api/v1/get\">show device info</a></li>";
  content += "<li><a href=\"/api/v1/restart\">restart device</a></li>";
  content += "<li>device settings:<br><br><form action=\"/api/v1/set\" method=\"get\">";
  content += "<input type=\"text\" id=\"ns\" name=\"ns\" maxlength=\"" + String(NS_LEN) + "\"><label for=\"ns\">&nbsp;namespace</label><br><br>";
  content += "<input type=\"text\" id=\"deviceId\" name=\"deviceId\" maxlength=\"" + String(DEVICE_ID_LEN) + "\"><label for=\"deviceId\">&nbsp;deviceId</label><br><br>";
  content += "<input type=\"text\" id=\"ssid\" name=\"ssid\" maxlength=\"" + String(SSID_LEN) + "\"><label for=\"ssid\">&nbsp;SSID</label><br><br>";
  content += "<input type=\"password\" id=\"pw\" name=\"pw\" maxlength=\"" + String(PW_LEN) + "\"><label for=\"pw\">&nbsp;WLAN pw</label><br><br>";
  content += "<input type=\"text\" id=\"mqttServer\" name=\"mqttServer\" maxlength=\"" + String(MQTT_SERVER_LEN) + "\"><label for=\"mqttServer\">&nbsp;MQTT Server (hostname or IP)</label><br><br>";
  content += "<input type=\"text\" id=\"mqttPort\" name=\"mqttPort\"><label for=\"mqttPort\">&nbsp;MQTT port</label><br><br>";
  content += "<input type=\"submit\" value=\"Submit\"></form></li></ul>";
  content += HTTP_FOOTER;
  return content;
}

void AM2H_Core::handleRestart() {
  debugMessage("AM2H_Core::handleRestart()","Restart-request received\n");
  const String content = "{\"message\":\"restart in 2 s!\"}";
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
    if (v.length()<1){ continue;}
    content += " " + a + ": " + v + "\n";

    if ( a == "deviceId" || a == "deviceid" ){
      debugMessage( "am2h_core->setDeviceId(" + v + ")\n" );
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
  debugMessage("AM2H_Core::handleApiSetRequest()",content);
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
  debugMessage("AM2H_Core::handleNotFound()",content);
  am2h_core->server_.send(404, ENCODING_PLAIN, content);
}

// ----------
// ---------- MQTT Utility Functions -----------------------------------------------------------
// ---------------------------------------------------------------------------------------------

void AM2H_Core::setupMqtt() {
  debugMessage("AM2H_Core::setupMqtt()",String(getMQTTServer()) + ":" + String(getMQTTPort()) );

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
  MqttTopic tp = AM2H_Core::parseMqttTopic(topic);
  debugMessage("AM2H_Core::mqttCallback()", "ns:"+tp.ns_+" dev:"+tp.dev_+" loc:"+tp.loc_+" srv:"+tp.srv_+" id:"+String(tp.id_)+" meas:"+tp.meas_ );

  String payloadString;
  for (int i = 0; i < length; i++) {
    payloadString += static_cast<char>(payload[i]);
  }

  if ( tp.srv_ == DEVICE_CFG_TOPIC && tp.id_ == 0xFF ){
    debugMessage("AM2H_Core::mqttCallback()", " config: " + payloadString + "\n");
    if ( tp.meas_ == "sampleRate" ){
      am2h_core->volatileSetupValues_.sampleRate = payloadString.toInt();
      am2h_core->mqttClient_.publish(getStatusTopic().c_str(), ONLINE_PROP_VAL, RETAINED);
    }
    if ( tp.meas_ == "i2cMuxAddr" ){
      am2h_core->volatileSetupValues_.i2cMuxAddr = AM2H_Helper::parse_hex<uint8_t>(payloadString);
    }
  } else {
      // send cfg message to Plugin
      int i=0;
      while ( auto p = am2h_core->plugins_[i++] ){
        if (p->getPlugin() == tp.srv_ ){
          if ( tp.id_<20 ) {
            p->config(ds[tp.id_],tp,payloadString);
          }
        }
      }
  }
}

void AM2H_Core::mqttReconnect() {
  if (timer_.mqttReconnect < millis()) {
    debugMessage("AM2H_Core::mqttReconnect()", "Attempting MQTT connection and unsubscribe topics. Wait for connection: " );
    mqttClient_.unsubscribe((getConfigTopic() + "#").c_str());
    
    if ( mqttClient_.connect(getDeviceId().c_str(), getStatusTopic().c_str(), 2, RETAINED, OFFLINE_PROP_VAL)) {
      debugMessage("AM2H_Core::mqttReconnect()", "connected\nPublish Status to " + getStatusTopic() );
      publishConfigDeviceStatus();    
      if ( !(updateRequired_ & MQTT_UPDATE_REQUIRED) ) {
        debugMessage("AM2H_Core::mqttReconnect()", "Subscribe config-topic " +  getConfigTopic() );
        mqttClient_.subscribe((getConfigTopic() + "#").c_str());
      }
      connStatus_ = MQTT_CLIENT_CONNECTED;
    } else {
      debugMessage( "failed with error code " + String(mqttClient_.state()) + " trying again in 1 second\n" );
      timer_.mqttReconnect = millis() + 1000;
      connStatus_ = WLAN_CONNECTED;
    }
  }
}

void AM2H_Core::subscribe(const char* topic){
  debugMessage("AM2H_Core::subscribe()", "Subscribe storage topic " +  String(topic) );
  mqttClient_.subscribe(topic);
  mqttClient_.loop();
}
void AM2H_Core::unsubscribe(const char* topic){
  debugMessage("AM2H_Core::unsubscribe()", "unsubscribe storage topic " +  String(topic) );
  mqttClient_.unsubscribe(topic);
  mqttClient_.loop();
}

void AM2H_Core::publishDeviceStatus(){
  am2h_core->mqttClient_.publish((getStorageTopic()+HEAP_PROP_NAME).c_str(), String(ESP.getFreeHeap()).c_str(), RETAINED);
  loopMqtt();
  am2h_core->mqttClient_.publish((getStorageTopic()+RUN_PROP_NAME).c_str(), String(millis()).c_str(), RETAINED);
  loopMqtt();
}

void AM2H_Core::publishConfigDeviceStatus(){
  publishDeviceStatus();
  mqttClient_.publish(getStatusTopic().c_str(), CONFIG_PROP_VAL, RETAINED);
  loopMqtt();
  am2h_core->mqttClient_.publish((getStorageTopic()+RESET_PROP_NAME).c_str(), ESP.getResetReason().c_str(), RETAINED);
  loopMqtt();
  am2h_core->mqttClient_.publish((getStorageTopic()+IPADDRESS_PROP_NAME).c_str(), WiFi.localIP().toString().c_str(), RETAINED);
  loopMqtt();
  am2h_core->mqttClient_.publish((getStorageTopic()+VERSION_PROP_NAME).c_str(), VERSION.c_str(), RETAINED);
  loopMqtt();
}

const String AM2H_Core::getStatusTopic() {
  return AM2H_Core::getStorageTopic() + STATUS_PROP_NAME;
}

const String AM2H_Core::getConfigTopic() {
  return am2h_core->getNamespace() + "/" + DEVICE_PROP_NAME + "/" + am2h_core->getDeviceId() + "/";
}

const String AM2H_Core::getStorageTopic() {
  return am2h_core->getNamespace() + "/" + STORAGE_PROP_NAME + "/" + am2h_core->getDeviceId() + "/";
}

const String AM2H_Core::getFullStorageTopic(const String id, const String srv, const String meas){
  return am2h_core->getNamespace() + "/" + STORAGE_PROP_NAME + "/" + am2h_core->getDeviceId() + "/" + id + "/" + srv + "/" + meas;
}


const String AM2H_Core::getDataTopic(const String loc, const String srv, const String id) {
  return am2h_core->getNamespace() + "/" + loc + "/" + srv + "/";
  // return am2h_core->getNamespace() + "/" + loc + "/" + am2h_core->getDeviceId() + "/" + srv + "/" + id + "/";
}

const bool AM2H_Core::isIntAvailable() const {
    return intAvailable_G;
}

const unsigned long AM2H_Core::getLastImpulseMillis() const {
    return lastImpulseMillis_G;
}

MqttTopic AM2H_Core::parseMqttTopic(char* topic){
  int i{0};
  String part[6];
  int p{0};

  while ( ( topic[i] != '\0' ) && ( p < 6 )) {
    if ( topic[i] != '/' ){
      part[p]+=topic[i];
    } else {
      ++p;
    }
    ++i;
  }

  return MqttTopic(part[0],part[1],part[2],part[3],part[4],part[5]);
}

// ----------
// ---------- Wire Utility Functions -----------------------------------------------------------
// ---------------------------------------------------------------------------------------------

void AM2H_Core::switchWire(uint32_t const addr) const {
  if (volatileSetupValues_.i2cMuxAddr==0){return;}
  uint8_t channel = (addr & 0x0F00) >> 8;
  if (channel > 7) return;
  Wire.beginTransmission(volatileSetupValues_.i2cMuxAddr);
  Wire.write(1 << channel);
  Wire.endTransmission();
}
