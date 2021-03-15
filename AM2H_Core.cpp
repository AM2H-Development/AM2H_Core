#include "AM2H_Core.h"
#include "AM2H_Core_Constants.h"

#define _SERIALDEBUG_  // enable serial debugging


AM2H_Core* am2h_core;

AM2H_Core::AM2H_Core(Plugin** plugins, PubSubClient& mqttClient, ESP8266WebServer& server):plugins_(plugins),mqttClient_(mqttClient),server_(server){
  status_=""; // status container for debugging messages
  updateRequired_ = NO_UPDATE_REQUIRED; // semaphore for system updates
  connStatus_ = CONN_UNKNOWN;
  String("192.168.178.100").toCharArray(persistentSetupValues_.mqttServer,MQTT_SERVER_LEN);
  // old: strcpy (persistentSetupValues_.mqttServer,"192.168.178.100");
  persistentSetupValues_.mqttPort = 1883;
  String("myDevice").toCharArray(persistentSetupValues_.deviceId,DEVICE_ID_LEN);
  // strcpy (persistentSetupValues_.deviceId, "myDevice");
  String("myHome").toCharArray(persistentSetupValues_.ns,NS_LEN);
  // strcpy (persistentSetupValues_.ns, "myHome");
  volatileSetupValues_.ssid=""; 
  volatileSetupValues_.pw="";
  // { "", "", "NONE", "myLoc"}; //, 1.0, 1.0, 10000000, 0.0, 0.0 };
  timer_.espRestart=0;
  timer_.mqttReconnect=0;
  timer_.wlanReconnect=0;

  lastImpulseMillis_ = 0;
  impulsesTotal_ =0;
  intAvailable_ = false;

  // attachInterrupt(IMPULSE_ISR_PIN, impulseISR, FALLING);
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
}


void AM2H_Core::loop(){
  loopServer();
  loopMqtt();

  // checkUpdateRequired();
}

void AM2H_Core::impulseISR()                                    // Interrupt service routine
{
  if ( millis() - am2h_core->lastImpulseMillis_ >= 50 )            // 50ms Entprellzeit
  {
    am2h_core->lastImpulseMillis_ = millis();                    // Zeit merken
    ++am2h_core->impulsesTotal_;
    //_ds.volumeAbsolut += _ds.volumePerTick;
    am2h_core->intAvailable_ = true;                            // Daten zum Senden vorhanden
    // dataSent = false;
  }
}

void AM2H_Core::checkUpdateRequired() {
  if (updateRequired_ != NO_UPDATE_REQUIRED) {
    // debugMessage("Update required: ");
    // debugMessage(String(updateRequired, BIN));
    // debugMessage("\n");
    if (updateRequired_ & MQTT_PUBLISH_MESSAGE) {
      // Check if needed: sendToMqtt(VOL_ABS_UPDATE);
      updateRequired_ ^= MQTT_PUBLISH_MESSAGE;
    }
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
      mqttClient_.setServer(getMQTTServer().c_str(), getMQTTPort());
      connStatus_ = WLAN_CONNECTED;
      updateRequired_ ^= MQTT_RESET_REQUIRED;
    }
    if ( (updateRequired_ & MQTT_UPDATE_REQUIRED) && (connStatus_ >= MQTT_CLIENT_CONNECTED) ) {
      debugMessage("MQTT_UPDATE_REQUIRED\n");
      // calcMinFlowMillis(); // Move to Plugin
      mqttPublishApiConfig();
      updateRequired_ ^= MQTT_UPDATE_REQUIRED;
    }
    if (updateRequired_ & ESP_RESET_REQUIRED) {
      debugMessage("ESP_RESET_REQUIRED\n");
      timer_.espRestart = millis() + 10000;
      updateRequired_ ^= ESP_RESET_REQUIRED;
      connStatus_ = DEV_RESTART_PENDING;
    }
  }

  if ( (timer_.wlanReconnect > 0) && (millis() > timer_.wlanReconnect) ) {
    debugMessage("Proceed WLAN reconnect!\n");
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
  timeout *= 2;
  debugMessage("WIFI - connecting:");

  while ( (WiFi.status() != WL_CONNECTED) && ( timeout != 0) ) {
    delay(500);
    timeout--;
    debugMessage(".");
  }

  if (timeout == 0) {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(getDeviceId());
    debugMessage("\n No connection established ... switiching to AP mode with SSID=");
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

  handleRootH=nullptr;  
  handleRootH = reinterpret_cast<ServerHandler>(&AM2H_Core::handleRoot);
  server_.on("/", handleRootH);

  server_.on(API_VERSION, handleApiRequest);
  server_.on(API_RESTART, handleRestart);
  server_.on(API_STATUS, handleStatus);
  server_.onNotFound(handleNotFound);
  server_.begin();
}

void AM2H_Core::loopServer(){
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
  debugMessage("Restart-request receiced");
  String content = "{\"message\":\"restart in 10 s!\"}";
  am2h_core->updateRequired_ |= ESP_RESET_REQUIRED;
  am2h_core->server_.send(200, ENCODING_JSON, content);
}

void AM2H_Core::handleStatus() {
  String content = "{\"status\":\"";
  content += am2h_core->status_;
  content += "\"}";
  am2h_core->server_.send(200, ENCODING_JSON, content);
}

void AM2H_Core::handleApiRequest() {
  debugMessage("API - request receiced with ");
  int params = 0;
  String content = "";
  const char* jsonPayload;

  if (am2h_core->server_.method() == HTTP_GET) {
    debugMessage("HTTP GET \n");
    am2h_core->handleApiGetRequest(content);
  }

  if (am2h_core->server_.method() == HTTP_POST) {
    debugMessage(" HTTP POST \n");
    for (uint8_t i = 0; i < am2h_core->server_.args(); i++) {
      if (am2h_core->server_.argName(i) == "plain") {
        jsonPayload = am2h_core->server_.arg(i).c_str(); params++;
        debugMessage("payload: ");
        debugMessage(jsonPayload);
      }
    }
    if (params != 1) {
      content =  "{\"";
      content += ERROR_PROP_NAME;
      content += "\":\"no valid json received!\"}";
    } else {
      am2h_core->handleApiPostRequest(content, jsonPayload );
    }
  }
#ifdef _SERIALDEBUG_
  Serial.print("Content: ");
  Serial.print(content);
  Serial.println();
#endif
  am2h_core->server_.send(200, ENCODING_JSON, content);
}

void AM2H_Core::handleApiGetRequest(String& content) {
  // jsonifySetupValues(content); TODO
}


void AM2H_Core::handleApiPostRequest(String& content, const char* jsonPayload ) {
  DynamicJsonDocument doc(JSON_CAPACITY);
  deserializeJson(doc, jsonPayload);
  String deviceId      = doc[DEVICE_PROP_NAME][DEVICE_ID_PROP_VAL];
  String ssid          = doc[WLAN_PROP_NAME][SSID_PROP_VAL];
  String pw            = doc[WLAN_PROP_NAME][PW_PROP_VAL];
  String mqttServer    = doc[MQTT_PROP_NAME][MQTT_SERVER_PROP_VAL];
  int    mqttPort      = doc[MQTT_PROP_NAME][MQTT_PORT_PROP_VAL];
  String ns            = doc[MQTT_PROP_NAME][NS_PROP_VAL];
  String loc           = doc[CFG_PROP_NAME] [0][LOC_PROP_VAL];
  String srv           = doc[CFG_PROP_NAME] [0][SERVICE_PROP_VAL];
  // float  volumePerTick = doc[CFG_PROP_NAME] [0][VOL_PER_TICK_PROP_VAL];
  // float  minFlow       = doc[CFG_PROP_NAME] [0][MIN_FLOW_PROP_VAL];
  // float  volumeAbsolut = doc[DATA_PROP_NAME][0][VOL_ABS_PROP_VAL];
  doc[MESSAGE_PROP_NAME] = "config request processed!";
  serializeJson(doc, content);

  if (deviceId != "null") {
    deviceId.toCharArray(persistentSetupValues_.deviceId, DEVICE_ID_LEN);
    updateRequired_ |= COMMIT_TO_EEPROM_REQUIRED | MQTT_UPDATE_REQUIRED | MQTT_RESET_REQUIRED;
  }
  if (ssid != "null") {
    volatileSetupValues_.ssid = ssid;
    updateRequired_ |= WLAN_RESET_REQUIRED;
  }
  if (pw != "null") {
    volatileSetupValues_.pw = pw;
    updateRequired_ |= WLAN_RESET_REQUIRED;
  }
  if (mqttServer != "null") {
    mqttServer.toCharArray(persistentSetupValues_.mqttServer, MQTT_SERVER_LEN);
    updateRequired_ |= COMMIT_TO_EEPROM_REQUIRED | MQTT_RESET_REQUIRED;
  }
  if (mqttPort) {
    setMQTTPort(mqttPort);
    updateRequired_ |= COMMIT_TO_EEPROM_REQUIRED | MQTT_RESET_REQUIRED;
  }
  if (ns != "null") {
    ns.toCharArray(persistentSetupValues_.ns, NS_LEN);
    updateRequired_ |= COMMIT_TO_EEPROM_REQUIRED | MQTT_UPDATE_REQUIRED | MQTT_RESET_REQUIRED;
  }
  /* To Plugin
  if (srv != "null") {
    srv.toCharArray(getService(), SERVICE_LEN);
    updateRequired_ |= MQTT_UPDATE_REQUIRED | MQTT_RESET_REQUIRED;
  }  if (loc != "null") {
    loc.toCharArray(getLocation(), LOC_LEN);
    updateRequired_ |= MQTT_UPDATE_REQUIRED | MQTT_RESET_REQUIRED;
  }

  */

  /*
  Needs to be moved to Plugin:
  if (volumePerTick) {
    _ds.volumePerTick = volumePerTick;
    updateRequired |= MQTT_UPDATE_REQUIRED;
  }
  if (minFlow) {
    _ds.minFlow = minFlow;
    updateRequired |= MQTT_UPDATE_REQUIRED;
  }
  if (volumeAbsolut) {
    _ds.volumeAbsolut = volumeAbsolut;
    updateRequired |= MQTT_PUBLISH_MESSAGE;
  }
  */
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
// ---------- JSON DOC Handler + Utility -------------------------------------------------------
// ---------------------------------------------------------------------------------------------

void AM2H_Core::jsonifySetupValues(String& jsonString) {
  DynamicJsonDocument doc(JSON_CAPACITY);
  JsonObject device = doc.createNestedObject(DEVICE_PROP_NAME);
  device[DEVICE_ID_PROP_VAL] = getDeviceId();
  JsonObject wlan = doc.createNestedObject(WLAN_PROP_NAME);
  wlan[SSID_PROP_VAL] = getSSID();
  wlan[PW_PROP_VAL] = getPW();
  JsonObject mqtt = doc.createNestedObject(MQTT_PROP_NAME);
  mqtt[MQTT_SERVER_PROP_VAL] = getMQTTServer();
  mqtt[MQTT_PORT_PROP_VAL] = getMQTTPort();
  mqtt[NS_PROP_VAL] = getNamespace();
  JsonArray cfg  = doc.createNestedArray(CFG_PROP_NAME);
  JsonObject cfg_0 = cfg.createNestedObject();
  // cfg_0[LOC_PROP_VAL] = getLocation();
  // cfg_0[SERVICE_PROP_VAL] = getService();
  // cfg_0[VOL_PER_TICK_PROP_VAL] = serialized(String(s.volumePerTick, 3));
  // cfg_0[MIN_FLOW_PROP_VAL] = serialized(String(s.minFlow, 2));
  JsonArray dta  = doc.createNestedArray(DATA_PROP_NAME);
  JsonObject dta_0 = dta.createNestedObject();
  // dta_0[VOL_ABS_PROP_VAL] = serialized(String(s.volumeAbsolut, 3));
  // dta_0[VOL_FLOW_PROP_VAL] = serialized(String(s.volumeFlow, 2));

  serializeJson(doc, jsonString);
#ifdef _SERIALDEBUG_
  Serial.print("JSON created:");
  //serializeJson(doc, Serial);
  Serial.println(jsonString);
#endif
}

// ----------
// ---------- MQTT Utility Functions -----------------------------------------------------------
// ---------------------------------------------------------------------------------------------
void AM2H_Core::setupMqtt() {
  mqttClient_.setServer(getMQTTServer().c_str(), getMQTTPort());
  // mqttCallbackH = reinterpret_cast<MqttHandler>(&AM2H_Core::mqttCallback);

  mqttClient_.setCallback(mqttCallbackH);
}

void AM2H_Core::loopMqtt() {
  if (!mqttClient_.connected() && ( connStatus_ >= WLAN_CONNECTED ) ) {
    mqttReconnect();
  } else {
    mqttClient_.loop();
  }
}

void AM2H_Core::mqttCallback(char* topic, uint8_t* payload, unsigned int length) {
  debugMessage( "Message arrived [" );
  debugMessage( topic );
  debugMessage( "]\n" );
  String topicString = String(topic);

  if ( topicString == getConfigTopic() ) {
    String s;
    debugMessage( "config: " );
    for (int i = 0; i < length; i++) {
      s += (char) payload[i];
    }
    debugMessage( s );
    debugMessage( " #\n" );
    DynamicJsonDocument doc(1500);
    deserializeJson(doc, payload, length);
    String loc           = doc[0][LOC_PROP_VAL];
    String srv           = doc[0][SERVICE_PROP_VAL];
    // float  volumePerTick = doc[0][VOL_PER_TICK_PROP_VAL];
    // float  minFlow       = doc[0][MIN_FLOW_PROP_VAL];
    
    mqttClient_.publish(getStatusTopic().c_str(), ONLINE_PROP_VAL, RETAINED);

    /* Need to be moved to Plugin

    if (loc != "null") {
      debugMessage( "Location set, " );
      loc.toCharArray(getLocation(), LOC_LEN);
    }
    if (srv != "null") {
      debugMessage( "Service set, " );
      srv.toCharArray(_ds.srv, SERVICE_LEN);
    }
    if (volumePerTick) {
      debugMessage( "volumePerTick set, " );
      _ds.volumePerTick = volumePerTick;
    }
    if (minFlow) {
      debugMessage( "minFlow set " );
      _ds.minFlow = minFlow;
    }

    calcMinFlowMillis();

    debugMessage("\nSubscribe to counter: " );
    debugMessage( getVolumeAbsolutTopic() );
    debugMessage("\n" );
    mqttClient.subscribe(getVolumeAbsolutTopic().c_str());
    connStatus_ = DEV_CONFIGURED;
    */
  }

  /* Need to be moved to Plugin:
  if ( topicString == getVolumeAbsolutTopic() ) {
    String s;
    debugMessage( "volumeAbsolutReceived: " );
    for (int i = 0; i < length; i++) {
      s += (char) payload[i];
    }
    debugMessage( s );
    debugMessage( " # Set _ds.volumeAbsolut to " );
    _ds.volumeAbsolut = s.toFloat();
    debugMessage(String(_ds.volumeAbsolut, 3));
    debugMessage("\n" );
  }
  */
}

void AM2H_Core::mqttReconnect() {
  if (timer_.mqttReconnect < millis()) {
    debugMessage( "Attempting MQTT connection and unsubscribe topics ..." );
    mqttClient_.unsubscribe(getConfigTopic().c_str());
    // mqttClient_.unsubscribe(getVolumeAbsolutTopic().c_str()); Move to Plugin!
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
        mqttClient_.subscribe(getConfigTopic().c_str());
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


String AM2H_Core::getStatusTopic() {
  return String(am2h_core->getNamespace()) + "/" + String(DEVICE_PROP_NAME) + "/" + String(am2h_core->getDeviceId()) + "/" + String(STATUS_PROP_NAME);
}


String AM2H_Core::getConfigTopic() {
  return String(am2h_core->getNamespace()) + "/" + DEVICE_PROP_NAME + "/" + String(am2h_core->getDeviceId()) + "/" + String(CFG_PROP_NAME);
}

void AM2H_Core::mqttPublishApiConfig() {
  String setupPayload;
  setupPayload  = "[{";
  setupPayload += "\"";
  setupPayload += LOC_PROP_VAL;
  setupPayload += "\":\"";
  // setupPayload += getLocation();
  setupPayload += "\", \"";
  setupPayload += SERVICE_PROP_VAL;
  setupPayload += "\":\"";
  // setupPayload += _ds.srv; // Move to Plugin
  setupPayload += "\", \"";
  // setupPayload += VOL_PER_TICK_PROP_VAL; // Move to Plugin
  setupPayload += "\":";
  // setupPayload += String(_ds.volumePerTick, 3); // Move to Plugin
  setupPayload += ", \"";
  // setupPayload += MIN_FLOW_PROP_VAL; // Move to Plugin
  setupPayload += "\":";
  // setupPayload += String(_ds.minFlow, 2); // Move to Plugin
  setupPayload += "}]";
  mqttClient_.publish(getConfigTopic().c_str(), setupPayload.c_str(), RETAINED);
  // mqttClient_.publish(getVolumeAbsolutTopic().c_str(), String(_ds.volumeAbsolut, 3).c_str(), RETAINED); // Move to Plugin
  mqttClient_.subscribe(getConfigTopic().c_str());
  mqttClient_.publish(getStatusTopic().c_str(), ONLINE_PROP_VAL, RETAINED);
  connStatus_ = DEV_CONFIGURED;

}