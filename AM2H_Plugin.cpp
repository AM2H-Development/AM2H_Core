#include <AM2H_Core.h>
#include "AM2H_Plugin.h"

struct Cfg {
    float volumePerTick=1.0;
    float minFlow=0.0;
    unsigned long minFlowMillis;
    float volumeAbsolut=0.0;
    float volumeFlow=0.0;
    unsigned long powerMillis = 0; // timestamp
} cfg{};

void TestPlugin::setup(){
  getCore().debugMessage("Setup");
  cfg.minFlow = 100.; // TODO
  // ...
    // setup
}

void TestPlugin::loop(){
  checkNewMeasure();
  checkZeroPower();
  getCore().getStatusTopic();
}

TestPlugin::TestPlugin(){
  srv_="test";
}

void TestPlugin::config(AM2H_Core& core, const char *key, const char *val) {
    // config
}

// ----------
// ---------- Counter Specific Functions -------------------------------------------------------
// ---------------------------------------------------------------------------------------------
void TestPlugin::calcMinFlowMillis() {
  if (cfg.minFlow == 0.0) {
    cfg.minFlowMillis = 10000000;
    return;
  }
  cfg.minFlowMillis = cfg.volumePerTick * 1000.0 / (cfg.minFlow / 60000.0);
}

void TestPlugin::calcPower() {
  if (cfg.powerMillis == 0) {
    cfg.powerMillis = millis();
    return;
  }
  float deltaPowerMinutes = millis() - cfg.powerMillis;
  deltaPowerMinutes /= 60000.0;
  cfg.volumeFlow = cfg.volumePerTick * 1000.0 / deltaPowerMinutes;

  core_->debugMessage( " # Set _ds.volumeFlow to " );
  core_->debugMessage(String(cfg.volumeFlow, 3));
  core_->debugMessage("\n" );

  cfg.powerMillis = millis();
}

void TestPlugin::checkNewMeasure() {
  if (core_->isIntAvailable()) {
    unsigned long timespan = millis() - core_->getLastImpulseMillis();
    if ( timespan >= 50 ) { // wait to debounce;
      if ( digitalRead(CORE_ISR_PIN) == HIGH ) { // wait for switch to open or 0,5s
        if (timespan >= 500) {
          cfg.volumeAbsolut += cfg.volumePerTick;
          calcPower();
          sendToMqtt(VOL_ABS_UPDATE | VOL_FLOW_UPDATE);
        }
        // dataSent = true; ???
        core_->resetInt();
      }
    }
  }
}

void TestPlugin::checkZeroPower() {
  if ( (cfg.volumeFlow > 0) && ((millis() - core_->getLastImpulseMillis()) >= cfg.minFlowMillis) ) {
    cfg.volumeFlow = 0;
    sendToMqtt(VOL_FLOW_UPDATE);
  }
}

void TestPlugin::sendToMqtt(byte updateT) {
  if (core_->getMqttClient().connected()) {
    char buf [10];
    if (updateT & VOL_ABS_UPDATE) {
      sprintf (buf, "%6.3f", cfg.volumeAbsolut);
      core_->getMqttClient().publish(getVolumeAbsolutTopic().c_str(), buf, RETAINED );
    }
    if (updateT & VOL_FLOW_UPDATE) {
      sprintf (buf, "%3.2f", cfg.volumeFlow);
      core_->getMqttClient().publish(getVolumeFlowTopic().c_str(), buf );
    }
  }
}

String TestPlugin::getVolumeAbsolutTopic() {
  //  return String(_ds.p.ns) + "/" + String(LOC_PROP_VAL) + "/" + String(_ds.loc) + "/" + String(_ds.p.deviceId) + "/" + String(_ds.srv) + "/" + String(VOL_ABS_PROP_VAL);
  return String(core_->getNamespace()) + "/" + String(LOC_PROP_VAL) + "/" + String("core_.getLocation()") + "/" + String(core_->getDeviceId()) + "/" + String(VOL_ABS_PROP_VAL);
}

String TestPlugin::getVolumeFlowTopic() {
  //  return String(_ds.p.ns) + "/" + String(LOC_PROP_VAL) + "/" + String(_ds.loc) + "/" + String(_ds.p.deviceId) + "/" + String(_ds.srv) + "/" + String(VOL_FLOW_PROP_VAL);
  return String(core_->getNamespace()) + "/" + String(LOC_PROP_VAL) + "/" + String("core_.getLocation()") + "/" + String(core_->getDeviceId()) + "/" + String(VOL_FLOW_PROP_VAL);
}
