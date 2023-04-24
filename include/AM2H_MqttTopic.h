#ifndef AM2H_MqttTopic_h
#define AM2H_MqttTopic_h
#include "Arduino.h"

class MqttTopic {
public:
    String ns_;
    String loc_;
    String dev_;
    String srv_;
    byte id_;
    String meas_;
    String value_;

    MqttTopic(String ns, String loc, String dev, String id, String srv, String meas): ns_(ns), loc_(loc), dev_(dev), srv_(srv), meas_(meas){
        if (id=="-") {
            id_=0xFF;
        } else {
            id_ = id.toInt();
        }
    }

    void setValue(const String value){
        value_=value;
    }
};

#endif