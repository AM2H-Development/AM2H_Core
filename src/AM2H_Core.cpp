#include "AM2H_Core.h"
// #define _SERIALDEBUG_  // enable serial debugging
// #define _NOLOGGING_ // disable logging

volatile uint32_t lastImpulseMillis_G{0};
volatile uint32_t impulsesTotal_G{0};
volatile bool intAvailable_G{false}; // Interupt available?

WiFiClient mqttWifiClient;
PubSubClient mqttClient(mqttWifiClient);
ESP8266WebServer server(80);

AM2H_Datastore ds[20];

// RAM:   [======    ]  60.1% (used 49216 bytes from 81920 bytes)
// Flash: [=====     ]  53.0% (used 403435 bytes from 761840 bytes)

// Interupt service routine
void impulseISR()
{
  if (millis() - lastImpulseMillis_G >= CORE_ISR_DEBOUNCE)
  {
    lastImpulseMillis_G = millis();
    intAvailable_G = true;
  }
}

AM2H_Core *am2h_core{nullptr};

AM2H_Core::AM2H_Core(AM2H_Plugin **plugins) : AM2H_Core(plugins, mqttClient, server){};

AM2H_Core::AM2H_Core(AM2H_Plugin **plugins, PubSubClient &mqttClient, ESP8266WebServer &server) : plugins_(plugins), mqttClient_(mqttClient), server_(server)
{
  mqttClient.setBufferSize(2500);
  volatileSetupValues_.i2cMuxAddr = 0;
  volatileSetupValues_.nickname = "";
  updateRequired_ = NO_UPDATE_REQUIRED; // semaphore for system updates
  connStatus_ = CONN_UNKNOWN;
  String(MQTT::DEFAULT_SERVER).toCharArray(persistentSetupValues_.mqttServer, MQTT_SERVER_LEN);
  persistentSetupValues_.mqttPort = MQTT::DEFAULT_PORT;
  String(MQTT::DEVICE).toCharArray(persistentSetupValues_.deviceId, DEVICE_ID_LEN);
  String(MQTT::NAMESPACE).toCharArray(persistentSetupValues_.ns, NS_LEN);
  volatileSetupValues_.ssid = WiFi.SSID();
  volatileSetupValues_.pw = "";
  volatileSetupValues_.sampleRate = 0;
  timer_.espRestart = 0;
  timer_.mqttReconnect = 0;
  timer_.wlanReconnect = 0;
  timer_.sendData = 0;
  am2h_core = this;
  mqttReconnectCounter = 0;
}

void AM2H_Core::setupCore()
{
  Serial.begin(115200);
  const auto CALLER = F("setupCore");
  String const message = "V=" + String(VERSION) + " MAC=" + WiFi.macAddress() + " RES=" + ESP.getResetReason();
  debugMessage(CALLER, message, DebugLogger::INFO);

  Serial.print(F("AM2H_Core Version = ") + String(VERSION) + "\n");
  Serial.print(F("Starting up - please wait "));
  for (int i = 3; i > 0; --i)
  {
    Serial.print('.');
    delay(1000);
  }
  Serial.print(F("\nrunning!\n"));
  setupEEPROM();
  Serial.print("ns: " + String(persistentSetupValues_.ns) + ", deviceId:" + String(persistentSetupValues_.deviceId) + "\n");

#ifndef _SERIALDEBUG_
  Serial.flush();
  Serial.end();
#endif

  pinMode(CORE_STATUS_LED, OUTPUT);

  setupWlan();
  setupServer();
  setupMqtt();
  setupWire();

  pinMode(CORE_ISR_PIN, INPUT_PULLUP);
  attachInterrupt(CORE_ISR_PIN, impulseISR, FALLING);

  int i = 0;
  while (auto p = plugins_[i++])
  {
    p->setupPlugin();
  }
}

void AM2H_Core::wait(uint32_t const del_millis)
{
  uint32_t now = millis();
  do
  {
    yield();
    loopMqtt();
    loopPlugins();
    checkTimerPublish();
    checkIntPublish();
  } while (millis() - now < del_millis);
}

void AM2H_Core::loopCore()
{
  loopServer();
  loopMqtt();
  checkUpdateRequired();
  checkTimerPublish();
  checkIntPublish();
  loopPlugins();
}

void AM2H_Core::loopPlugins()
{
  for (int datastoreIndex = 0; datastoreIndex < 20; ++datastoreIndex)
  {
    if (auto p = ds[datastoreIndex].self)
    {
      p->loopPlugin(ds[datastoreIndex], datastoreIndex);
      yield();
    }
  }
}

void AM2H_Core::checkIntPublish()
{
  if (!intAvailable_G) return;
  if (millis() - lastImpulseMillis_G < ICOUNTER_MIN_IMPULSE_TIME_MS) return;

  noInterrupts();
  intAvailable_G = false;
  interrupts();

  ++impulsesTotal_G;
  for (int i = 0; i < 20; ++i)
  {
    if (auto p = ds[i].self)
    {
      p->interruptPublish(ds[i], mqttClient_, getDataTopic(ds[i].loc, ds[i].self->getSrv(), String(i)), i);
    }
  }
}

void AM2H_Core::checkTimerPublish()
{
  if (volatileSetupValues_.sampleRate > 0)
  {
    if (millis() - timer_.sendData > volatileSetupValues_.sampleRate * 1000)
    {
      timer_.sendData += volatileSetupValues_.sampleRate * 1000;
      publishDeviceStatus();
      for (int i = 0; i < 20; ++i)
      {
        if (auto p = ds[i].self)
        {
          p->timerPublish(ds[i], mqttClient_, getDataTopic(ds[i].loc, ds[i].self->getSrv(), String(i)), i);
          loopMqtt();
        }
      }
    }
  }
}

void AM2H_Core::checkUpdateRequired()
{
  const auto CALLER = F("checkUpdateRequired");

  if (mqttReconnectCounter > MQTT_RECONNECT_REBOOT_LIMIT && connStatus_ != DEV_RESTART_PENDING)
  {
    updateRequired_ |= ESP_RESET_REQUIRED;
  }

  if (connStatus_ == WLAN_AP_PROV && millis() > (WLAN_TIMEOUT * 3 * 1000) && WiFi.softAPgetStationNum() == 0)
  {
    updateRequired_ |= ESP_RESET_REQUIRED;
  }

  if (updateRequired_ != NO_UPDATE_REQUIRED)
  {
    if (updateRequired_ & COMMIT_TO_EEPROM_REQUIRED)
    {
      debugMessage(CALLER, F("COMMIT_TO_EEPROM_REQUIRED"), DebugLogger::INFO);
      updateRequired_ &= ~COMMIT_TO_EEPROM_REQUIRED;
      writeEEPROM();
    }
    if (updateRequired_ & WLAN_RESET_REQUIRED)
    {
      debugMessage(CALLER, F("WLAN_RESET_REQUIRED"), DebugLogger::INFO);
      timer_.wlanReconnect = millis() + 1000;
      updateRequired_ &= ~WLAN_RESET_REQUIRED;
    }
    if (updateRequired_ & MQTT_RESET_REQUIRED && (connStatus_ >= WLAN_CONNECTED))
    {
      debugMessage(CALLER, F("MQTT_RESET_REQUIRED"), DebugLogger::INFO);
      mqttClient_.disconnect();
      mqttClient_.setServer(getMQTTServer(), getMQTTPort());
      connStatus_ = WLAN_CONNECTED;
      updateRequired_ &= ~MQTT_RESET_REQUIRED;
    }
    if (updateRequired_ & ESP_RESET_REQUIRED)
    {
      debugMessage(CALLER, F("ESP_RESET_REQUIRED"), DebugLogger::INFO);
      timer_.espRestart = millis() + 2000;
      updateRequired_ &= ~ESP_RESET_REQUIRED;
      connStatus_ = DEV_RESTART_PENDING;
      pinMode(CORE_STATUS_LED, INPUT_PULLUP);
    }
  }

  if ((timer_.wlanReconnect > 0) && (millis() > timer_.wlanReconnect))
  {
    debugMessage(CALLER, F("Proceed WLAN reconnect! ") + getSSID() + " : " + getPW(), DebugLogger::INFO);
    timer_.wlanReconnect = 0;
    restartWlan(getSSID(), getPW());
  }

  if ((timer_.espRestart > 0) && (millis() > timer_.espRestart))
  {
    ESP.restart();
  }
}

// ----------
// ---------- EEPROM Utility Functions ---------------------------------------------------------
// ---------------------------------------------------------------------------------------------
void AM2H_Core::setupEEPROM()
{
  String message = "Size=" + String(sizeof(PersistentSetupContainer));

  EEPROM.begin(sizeof(PersistentSetupContainer));

  if (EEPROM.percentUsed() >= 0)
  {
    EEPROM.get(0, persistentSetupValues_);
    message += F(" FlashUse=") + String(EEPROM.percentUsed()) + "%";
  }
  else
  {
    message += F(" FlashWrite=");
    message += writeEEPROM();
  }
  message += " ns=" + String(persistentSetupValues_.ns) + ", deviceId=" + String(persistentSetupValues_.deviceId);

  debugMessage(F("setupEEPROM"), message, DebugLogger::INFO);
}

bool AM2H_Core::writeEEPROM()
{
  EEPROM.put(0, persistentSetupValues_);
  return EEPROM.commit();
}

// ----------
// ---------- Debug Message Handler ------------------------------------------------------------
// ---------------------------------------------------------------------------------------------

void const AM2H_Core::debugMessage(const String &caller, const String &message, const bool newline, const bool infoOrError)
{
#ifdef _NOLOGGING_
  return;
#endif

  if (!am2h_core) return;

  if (millis() > am2h_core->thershold && infoOrError == DebugLogger::INFO)
  {
    return;
  }

  if (infoOrError == DebugLogger::ERROR)
  {
    am2h_core->thershold = millis() + LOG_INFO_THRESHOLD;
  }

  am2h_core->logbook[am2h_core->logpos].ts = millis();
  am2h_core->logbook[am2h_core->logpos].freeHeap = ESP.getFreeHeap();

  am2h_core->logbook[am2h_core->logpos].error = infoOrError;

  auto c = strncpy(am2h_core->logbook[am2h_core->logpos].caller, caller.c_str(), LOG_CALLER_LENGTH);
  c[LOG_CALLER_LENGTH - 1] = 0;

  auto m = strncpy(am2h_core->logbook[am2h_core->logpos].message, message.c_str(), LOG_MESSAGE_LENGTH);
  m[LOG_MESSAGE_LENGTH - 1] = 0;

  ++am2h_core->logpos;
  if (am2h_core->logpos == LOG_LENGTH)
  {
    am2h_core->logpos = 0;
  }

#ifdef _SERIALDEBUG_
  Serial.print(newMessage);
#endif
}

// ----------
// ---------- WLAN Utility Functions -----------------------------------------------------------
// ---------------------------------------------------------------------------------------------

void AM2H_Core::setupWlan()
{
  WiFi.mode(WIFI_STA);
  WiFi.persistent(false);
  WiFi.begin();
  WiFi.hostname(getDeviceId());
  connectWlan(WLAN_TIMEOUT);
}

void AM2H_Core::restartWlan(String ssid, String pw)
{
  connStatus_ = CONN_UNKNOWN;
  WiFi.reconnect();
  WiFi.mode(WIFI_STA);
  WiFi.persistent(true);
  WiFi.begin(ssid, pw);
  connectWlan(WLAN_TIMEOUT);
}

void AM2H_Core::connectWlan(int timeout)
{
  String message = F("AP=") + WiFi.SSID();
  bool errOrInfo = DebugLogger::INFO;

  bool onOffLed{0};

  timeout *= 2;
  while ((WiFi.status() != WL_CONNECTED) && (timeout != 0))
  {
    digitalWrite(CORE_STATUS_LED, onOffLed);
    onOffLed = !onOffLed;
    uint32_t t0 = millis();
    while (millis() - t0 < 500) { yield(); }
    timeout--;
  }
  digitalWrite(CORE_STATUS_LED, LOW);

  if (timeout == 0)
  {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(getDeviceId());
    message += F(" timeout, switching to STA=") + getDeviceId();
    errOrInfo = DebugLogger::ERROR;
    connStatus_ = WLAN_AP_PROV;
  }
  else
  {
    message += F(" connected, IP=") + WiFi.localIP().toString();
    connStatus_ = WLAN_CONNECTED;
  }
  debugMessage(F("connectWLAN"), message, errOrInfo);
}

// ----------
// ---------- Server Response Handler + Utility ------------------------------------------------
// ---------------------------------------------------------------------------------------------
void AM2H_Core::setupServer()
{
  debugMessage(F("setupServer"), F("setting up handlers"), DebugLogger::INFO);
  server_.on("/", handleRoot);
  server_.on(HTTP_API_V1_SET, handleApiSetRequest);
  server_.on(HTTP_API_V1_GET, handleApiGetRequest);
  server_.on(HTTP_API_V1_RESTART, handleRestart);
  server_.on(HTTP_API_V1_STATUS, handleStatus);
  // server_.on(HTTP_API_V1_PLUGIN, handlePlugin);
  server_.onNotFound(handleNotFound);
  server_.begin();
}

inline void AM2H_Core::loopServer()
{
  server_.handleClient();
}

void AM2H_Core::handleRoot()
{
  debugMessage(F("handleRoot"), String((am2h_core->server_.method() == HTTP_GET) ? "GET" : "POST") + " INDEX", DebugLogger::INFO);
  am2h_core->server_.setContentLength(CONTENT_LENGTH_UNKNOWN);
  am2h_core->server_.send(200, ENCODING_HTML, "");
  am2h_core->server_.sendContent(HTTP_HEADER);
  am2h_core->server_.sendContent(F("<h1>AM2H_Core</h1><p>deviceId: <b>"));
  am2h_core->server_.sendContent(am2h_core->persistentSetupValues_.deviceId);
  am2h_core->server_.sendContent(F("</b><br/>nickname: <b>"));
  am2h_core->server_.sendContent(am2h_core->volatileSetupValues_.nickname);
  am2h_core->server_.sendContent(F("</b><br/>fwVersion: <b>" VERSION "</b><br/>MAC: <b>"));
  am2h_core->server_.sendContent(WiFi.macAddress());
  am2h_core->server_.sendContent(F("</b></p><ul>"
    "<li><a href=\"/api/v1/status\">show status logs</a></li>"
    "<li><a href=\"/api/v1/get\">show device info</a></li>"
    "<li><a href=\"/api/v1/restart\">restart device</a></li>"
    "<li>device settings:<br><br><form action=\"/api/v1/set\" method=\"get\">"
    "<input type=\"text\" id=\"deviceId\" name=\"deviceId\" maxlength=\""));
  am2h_core->server_.sendContent(String(DEVICE_ID_LEN));
  am2h_core->server_.sendContent(F("\"><label for=\"deviceId\">&nbsp;deviceId</label><br/><br/>"
    "<input type=\"text\" id=\"ns\" name=\"ns\" maxlength=\""));
  am2h_core->server_.sendContent(String(NS_LEN));
  am2h_core->server_.sendContent(F("\"><label for=\"ns\">&nbsp;namespace</label><br/><br/>"
    "<input type=\"text\" id=\"mqttServer\" name=\"mqttServer\" maxlength=\""));
  am2h_core->server_.sendContent(String(MQTT_SERVER_LEN));
  am2h_core->server_.sendContent(F("\"><label for=\"mqttServer\">&nbsp;MQTT Server (hostname or IP)</label><br/><br/>"
    "<input type=\"text\" id=\"mqttPort\" name=\"mqttPort\">"
    "<label for=\"mqttPort\">&nbsp;MQTT port</label><br/><br/>"
    "<input type=\"text\" id=\"ssid\" name=\"ssid\" maxlength=\""));
  am2h_core->server_.sendContent(String(SSID_LEN));
  am2h_core->server_.sendContent(F("\"><label for=\"ssid\">&nbsp;SSID</label><br/><br/>"
    "<input type=\"password\" id=\"pw\" name=\"pw\" maxlength=\""));
  am2h_core->server_.sendContent(String(PW_LEN));
  am2h_core->server_.sendContent(F("\"><label for=\"pw\">&nbsp;WLAN pw</label><br/><br/>"
    "<input type=\"submit\" value=\"Submit\"></form></li></ul><ul>"));
  int i = -1;
  while (auto p = am2h_core->plugins_[++i])
  {
    am2h_core->server_.sendContent(F("<li><a href=\"/api/v1/plugin/"));
    am2h_core->server_.sendContent(String(i));
    am2h_core->server_.sendContent(F("/\">"));
    am2h_core->server_.sendContent(p->getHtmlTabName());
    am2h_core->server_.sendContent(F("</a></li>"));
  }
  am2h_core->server_.sendContent(F("</ul>"));
  am2h_core->server_.sendContent(HTTP_FOOTER);
}

void AM2H_Core::handleRestart()
{
  debugMessage(F("handleRestart"), F("Restart-request received"), DebugLogger::INFO);
  const String content = F("{\"message\":\"restart in 2 s!\"}");
  am2h_core->updateRequired_ |= ESP_RESET_REQUIRED;
  am2h_core->server_.send(200, ENCODING_JSON, content);
}

void AM2H_Core::handleStatus()
{
  am2h_core->thershold = millis() + LOG_INFO_THRESHOLD;
  am2h_core->server_.setContentLength(CONTENT_LENGTH_UNKNOWN);
  am2h_core->server_.send(200, ENCODING_JSON, "");
  am2h_core->server_.sendContent(F("{\"statusLog\":\"\n"));

  auto logpos = am2h_core->logpos;
  for (size_t i = 0; i < LOG_LENGTH; ++i)
  {
    auto logEntry = am2h_core->logbook[logpos++];
    if (logEntry.ts > 0)
    {
      uint8_t callerLen = strnlen(logEntry.caller, LOG_CALLER_LENGTH);
      String entry;
      entry.reserve(LOG_CALLER_LENGTH + LOG_MESSAGE_LENGTH + 32);
      entry += logEntry.error == DebugLogger::INFO ? "  " : "! ";
      entry += '[';
      entry += String(logEntry.ts / 1000., 3);
      entry += F("] [H:");
      entry += String(logEntry.freeHeap);
      entry += F("] ");
      entry += logEntry.caller;
      for (uint8_t j = callerLen; j < LOG_CALLER_LENGTH; ++j) entry += ' ';
      entry += F("-> ");
      entry += logEntry.message;
      entry += '\n';
      am2h_core->server_.sendContent(entry);
    }
    if (logpos == LOG_LENGTH) logpos = 0;
  }
  am2h_core->server_.sendContent(F("\"}"));
}

void AM2H_Core::handlePlugin(const int id)
{
  int max_plugin = 0;
  while (am2h_core->plugins_[max_plugin++])
  {
  }
  --max_plugin;

  String content;
  content += HTTP_HEADER;
  content += F("<h1><a href=\"/\">AM2H_Core</a></h1><p>deviceId: <b>") + String(am2h_core->persistentSetupValues_.deviceId) + F("</b><br/>nickname: <b>") + String(am2h_core->volatileSetupValues_.nickname);
  content += F("</b><br/>fwVersion: <b>") + String(VERSION);
  content += F("</b><br/>MAC: <b>") + WiFi.macAddress() + F("</b></p>");
  if ((id >= 0) && (id < max_plugin))
  {
    content += am2h_core->plugins_[id]->getHtmlTabContent();
  }
  content += HTTP_FOOTER;
  am2h_core->server_.send(200, ENCODING_HTML, content);
}

void AM2H_Core::handleApiGetRequest()
{
  String content = F("{\n\"deviceId\":\"") + am2h_core->getDeviceId();
  content += F("\",\n\"nickname\":\"") + am2h_core->getNickname();
  content += F("\",\n\"MAC\":\"") + WiFi.macAddress();
  content += F("\",\n\"ssid\":\"") + am2h_core->getSSID();
  content += F("\",\n\"pw\":\"********\",\n\"mqttServer\":\"") + String(am2h_core->getMQTTServer());
  content += F("\",\n\"mqttPort\":\"") + String(am2h_core->getMQTTPort());
  content += F("\",\n\"ns\":\"") + am2h_core->getNamespace() + F("\"\n}");
  am2h_core->server_.send(200, ENCODING_JSON, content);
}

void AM2H_Core::handleApiSetRequest()
{
  String content = F("API set ");
  content += (am2h_core->server_.method() == HTTP_GET) ? "GET " : "POST ";
  content += am2h_core->server_.uri();
  content += F(" args=");
  content += am2h_core->server_.args();

  for (uint8_t i = 0; i < am2h_core->server_.args(); i++)
  {
    auto a = am2h_core->server_.argName(i);
    auto v = am2h_core->server_.arg(i);
    if (v.length() < 1)
    {
      continue;
    }
    content += " " + a + ":" + v + ";";

    if (a.equalsIgnoreCase("deviceId"))
    {
      am2h_core->setDeviceId(v);
    }
    if (a.equalsIgnoreCase("ssid"))
    {
      am2h_core->setSSID(v);
    }
    if (a.equalsIgnoreCase("pw"))
    {
      am2h_core->setPW(v);
    }
    if (a.equalsIgnoreCase("mqttServer"))
    {
      am2h_core->setMQTTServer(v);
    }
    if (a.equalsIgnoreCase("mqttPort"))
    {
      am2h_core->setMQTTPort(v);
    }
    if (a.equalsIgnoreCase("ns"))
    {
      am2h_core->setNamespace(v);
    }
  }
  debugMessage(F("handleApiSetRequest"), content, DebugLogger::INFO);
  am2h_core->server_.send(200, ENCODING_PLAIN, content);
}

void AM2H_Core::handleNotFound()
{
  // Check HTTP_API_V1_PLUGIN
  int len = String(HTTP_API_V1_PLUGIN).length();
  String uri = am2h_core->server_.uri().substring(0, len);
  if (uri.equalsIgnoreCase(HTTP_API_V1_PLUGIN))
  {
    String id;
    uri = am2h_core->server_.uri().substring(len);
    for (unsigned int i = 0; i < uri.length(); ++i)
    {
      char d = uri.charAt(i);
      if (d == '/')
        break;
      id += d;
    }
    int id_int = id.toInt();
    debugMessage(F("handleNotFound"), F("plugin router-> id=") + id, DebugLogger::INFO);
    handlePlugin(id_int);
    return;
  }

  // Handle 404
  String content = F("Not Found! ");
  content += (am2h_core->server_.method() == HTTP_GET) ? "GET " : "POST ";
  content += am2h_core->server_.uri();
  content += F(" args=");
  content += am2h_core->server_.args();
  for (uint8_t i = 0; i < am2h_core->server_.args(); i++)
  {
    content += " " + am2h_core->server_.argName(i) + ": " + am2h_core->server_.arg(i) + ";";
  }
  debugMessage(F("handleNotFound"), content, DebugLogger::INFO);
  am2h_core->server_.send(404, ENCODING_PLAIN, content);
}

// ----------
// ---------- MQTT Utility Functions -----------------------------------------------------------
// ---------------------------------------------------------------------------------------------

void AM2H_Core::setupMqtt()
{
  debugMessage(F("setupMqtt"), String(getMQTTServer()) + ":" + String(getMQTTPort()), DebugLogger::INFO);

  mqttClient_.setServer(getMQTTServer(), getMQTTPort());
  mqttClient_.setCallback(AM2H_Core::mqttCallback);
  mqttClient_.setKeepAlive(10);
  mqttClient_.setSocketTimeout(5);
}

void AM2H_Core::loopMqtt()
{
  if (connStatus_ >= WLAN_CONNECTED)
  {
    if (!mqttClient_.connected())
    {
      mqttReconnect();
    }
    else
    {
      if (!mqttClient_.loop())
      {
        debugMessage(F("loopMqtt"), F("loop() failed, forcing reconnect"), DebugLogger::ERROR);
        mqttClient_.disconnect();
        connStatus_ = WLAN_CONNECTED;
      }
      else if (millis() - timer_.mqttActivity > MQTT_ZOMBIE_TIMEOUT_MS)
      {
        debugMessage(F("loopMqtt"), F("zombie connection detected, forcing reconnect"), DebugLogger::ERROR);
        ++mqttReconnectCounter;
        mqttClient_.disconnect();
        connStatus_ = WLAN_CONNECTED;
      }
    }
  }
}

void AM2H_Core::mqttCallback(const char *topic, uint8_t *payload, unsigned int length)
{
  am2h_core->timer_.mqttActivity = millis();
  MqttTopic tp = AM2H_Core::parseMqttTopic(topic);
  debugMessage(F("mqttCallback"), "ns=" + tp.ns_ + " dev=" + tp.dev_ + " loc=" + tp.loc_ + " srv=" + tp.srv_ + " id=" + String(tp.id_) + " meas=" + tp.meas_ + "|", DebugLogger::INFO);

  String payloadString;
  for (unsigned int i = 0; i < length; i++)
  {
    payloadString += static_cast<char>(payload[i]);
  }

  if (tp.srv_ == DEVICE_CFG_TOPIC && tp.id_ == 0xFF)
  {
    if (tp.meas_.equalsIgnoreCase("sampleRate"))
    {
      am2h_core->volatileSetupValues_.sampleRate = payloadString.toInt();
      am2h_core->mqttClient_.publish(getStatusTopic().c_str(), ONLINE_PROP_VAL, RETAINED);
      am2h_core->loopMqtt();
      debugMessage(F("mqttCallback"), F("set sampleRate=") + String(am2h_core->volatileSetupValues_.sampleRate), DebugLogger::INFO);
    }
    if (tp.meas_.equalsIgnoreCase("i2cMuxAddr"))
    {
      am2h_core->volatileSetupValues_.i2cMuxAddr = AM2H_Helper::parse_hex<uint8_t>(payloadString);
      debugMessage(F("mqttCallback"), F("set i2cMuxAddr=") + String(am2h_core->volatileSetupValues_.i2cMuxAddr), DebugLogger::INFO);
      // am2h_core->scan();
    }
    if (tp.meas_.equalsIgnoreCase("nickname"))
    {
      am2h_core->volatileSetupValues_.nickname = payloadString;
      debugMessage(F("mqttCallback"), F("set nickname=") + String(am2h_core->volatileSetupValues_.nickname), DebugLogger::INFO);
    }
  }
  else
  {
    // send cfg message to Plugin
    int i = 0;
    while (auto p = am2h_core->plugins_[i++])
    {
      if (p->getPlugin() == tp.srv_)
      {
        if (tp.id_ < 20)
        {
          p->config(ds[tp.id_], tp, payloadString);
        }
      }
    }
  }
}

void AM2H_Core::mqttReconnect()
{
  if (timer_.mqttReconnect > millis())
    return;

  ++mqttReconnectCounter;

  debugMessage(F("mqttReconnect"), F("Attempting MQTT con. and unsubscribe topics"), DebugLogger::INFO);
  // mqttClient_.unsubscribe((getConfigTopic() + "#").c_str());

  if (mqttClient_.connect(getDeviceId().c_str(), getStatusTopic().c_str(), 2, RETAINED, OFFLINE_PROP_VAL))
  {
    debugMessage(F("mqttReconnect"), F("ok, publish Status to ") + getStatusTopic(), DebugLogger::INFO);
    publishConfigDeviceStatus();
    if (!(updateRequired_ & MQTT_UPDATE_REQUIRED))
    {
      debugMessage(F("mqttReconnect"), F("subscribe config topic ") + getConfigTopic(), DebugLogger::INFO);
      mqttClient_.subscribe((getConfigTopic() + "#").c_str());
    }
    mqttReconnectCounter = 0;
    timer_.mqttActivity = millis();
    connStatus_ = MQTT_CLIENT_CONNECTED;
    pinMode(CORE_STATUS_LED, INPUT_PULLUP);
  }
  else
  {
    debugMessage(F("mqttReconnect"), F("failed with error ") + String(mqttClient_.state()) + "(" + String(mqttReconnectCounter) + ")", DebugLogger::ERROR);
    timer_.mqttReconnect = millis() + 5000;
    connStatus_ = WLAN_CONNECTED;
    pinMode(CORE_STATUS_LED, OUTPUT);
    digitalWrite(CORE_STATUS_LED, LOW);
  }
}

void AM2H_Core::subscribe(const char *topic)
{
  debugMessage(F("subscribe "), F("subscribe ")+ String(topic), DebugLogger::INFO);
  mqttClient_.subscribe(topic);
  mqttClient_.loop();
}
void AM2H_Core::unsubscribe(const char *topic)
{
  debugMessage(F("unsubscribe "), F("unsubscribe ") + String(topic), DebugLogger::INFO);
  mqttClient_.unsubscribe(topic);
  mqttClient_.loop();
}

void AM2H_Core::publishDeviceStatus()
{
  am2h_core->mqttClient_.publish((getStorageTopic() + HEAP_PROP_NAME).c_str(), String(ESP.getFreeHeap()).c_str(), RETAINED);
  loopMqtt();
  am2h_core->mqttClient_.publish((getStorageTopic() + RUN_PROP_NAME).c_str(), String(millis()).c_str(), RETAINED);
  loopMqtt();
  am2h_core->mqttClient_.publish((getStorageTopic() + MQTT_RECONNECT_PROP_NAME).c_str(), String(mqttReconnectCounter).c_str(), RETAINED);
  loopMqtt();
}

void AM2H_Core::publishConfigDeviceStatus()
{
  publishDeviceStatus();
  mqttClient_.publish(getStatusTopic().c_str(), CONFIG_PROP_VAL, RETAINED);
  loopMqtt();
  am2h_core->mqttClient_.publish((getStorageTopic() + RESET_PROP_NAME).c_str(), ESP.getResetReason().c_str(), RETAINED);
  loopMqtt();
  am2h_core->mqttClient_.publish((getStorageTopic() + IPADDRESS_PROP_NAME).c_str(), WiFi.localIP().toString().c_str(), RETAINED);
  loopMqtt();
  am2h_core->mqttClient_.publish((getStorageTopic() + VERSION_PROP_NAME).c_str(), VERSION, RETAINED);
  loopMqtt();
}

const String AM2H_Core::getStatusTopic()
{
  return AM2H_Core::getStorageTopic() + STATUS_PROP_NAME;
}

const String AM2H_Core::getConfigTopic()
{
  return am2h_core->getNamespace() + "/" + DEVICE_PROP_NAME + "/" + am2h_core->getDeviceId() + "/";
}

const String AM2H_Core::getStorageTopic()
{
  return am2h_core->getNamespace() + "/" + STORAGE_PROP_NAME + "/" + am2h_core->getDeviceId() + "/";
}

const String AM2H_Core::getFullStorageTopic(const String id, const String srv, const String meas)
{
  return am2h_core->getNamespace() + "/" + STORAGE_PROP_NAME + "/" + am2h_core->getDeviceId() + "/" + id + "/" + srv + "/" + meas;
}

const String AM2H_Core::getDataTopic(const String loc, const String srv, const String id)
{
  return am2h_core->getNamespace() + "/" + loc + "/" + srv + "/";
}

const bool AM2H_Core::isIntAvailable() const
{
  return intAvailable_G;
}

const uint32_t AM2H_Core::getLastImpulseMillis() const
{
  return lastImpulseMillis_G;
}

MqttTopic AM2H_Core::parseMqttTopic(const char *topic)
{
  int i{0};
  String part[6];
  int p{0};

  while ((topic[i] != '\0') && (p < 6))
  {
    if (topic[i] != '/')
    {
      part[p] += topic[i];
    }
    else
    {
      ++p;
    }
    ++i;
  }

  return MqttTopic(part[0], part[1], part[2], part[3], part[4], part[5]);
}

// ----------
// ---------- Wire Utility Functions -----------------------------------------------------------
// ---------------------------------------------------------------------------------------------
void AM2H_Core::setupWire()
{
  Wire.begin(CORE_SDA, CORE_SCL);
}

void AM2H_Core::scan() const
{
  String message;
  for (uint8_t t = 0; t < 8; ++t)
  {
    switchWire(t << 8);
    message += F(" | TCA Port #") + String(t);

    for (uint8_t addr = 0; addr <= 127; ++addr)
    {
      if (addr == volatileSetupValues_.i2cMuxAddr)
        continue;
      Wire.beginTransmission(addr);
      int response = Wire.endTransmission();
      if (response == 0)
      {
        message += F(" Found I2C 0x") + String(addr, HEX) + "; ";
      }
    }
  }
  AM2H_Core::debugMessage(F("AM2H_Core::scan"), message, DebugLogger::INFO);
}

void AM2H_Core::switchWire(uint32_t const addr) const
{
  if (volatileSetupValues_.i2cMuxAddr == 0)
  {
    return;
  }
  uint8_t channel = (addr & 0x0F00) >> 8;
  if (channel > 7)
    return;
  Wire.beginTransmission(volatileSetupValues_.i2cMuxAddr);
  Wire.write(1 << channel);
  Wire.endTransmission();
}

/*
void AM2H_Core::i2cReset()
{

  AM2H_Core::debugMessageNl("AM2H_Core::i2cReset()","start", DebugLogger::ERROR);

  pinMode(CORE_SDA, INPUT_PULLUP); // Make SDA (data) and SCL (clock) pins Inputs with pullup.
  pinMode(CORE_SCL, INPUT_PULLUP);

  wait(500);
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
          wait(100);
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
  AM2H_Core::debugMessageNl("AM2H_Core::i2cReset()","end", DebugLogger::ERROR);

}*/
