#ifndef AM2H_MqttTopic_h
#define AM2H_MqttTopic_h
#include "Arduino.h"

struct MqttTopic {
    String ns_;
    String loc_;
    String dev_;
    String srv_;
    String id_;
    String meas_;

    MqttTopic(String ns, String loc, String dev, String srv, String id, String meas): ns_(ns), loc_(loc), dev_(dev), srv_(srv), id_(id), meas_(meas){}
};

#endif