#ifndef AM2H_Helper_h
#define AM2H_Helper_h
#include "Arduino.h"

namespace AM2H_Helper {
    template <class T> const T parse_hex(const String& string){
      T number{0};
      uint32_t shift{0};
      int len = string.length();
      for (int pos=0;  pos<len; ++pos){
          char c = string.charAt(len-pos-1);
          if ( c >= '0' && c <='9' ){ number += (c-'0') << shift; }
          if ( c >= 'A' && c <= 'F' ){ number+= (c-'A'+10) << shift;}
          if ( c >= 'a' && c <= 'f' ){ number+= (c-'a'+10) << shift;}
          if ( c=='x' ) {break;}
          shift+=4;
      }
      return number;
    }
    void parse_location(char* loc, const String parse );
};

#endif