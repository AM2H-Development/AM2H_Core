#include "AM2H_Helper.h"
#include "AM2H_Datastore.h"

namespace AM2H_Helper {
  void parse_location(char* loc, const String parse ){
    const uint8_t len = (parse.length()>= AM2H_Datastore::LOC_MAX_LEN)? AM2H_Datastore::LOC_MAX_LEN-1 : parse.length();
    for (int i=0; i < len; ++i){
      loc[i]= parse.charAt(i);
    }
    loc[len]='\0';
  }

  bool stringToBool(const String s){
    String temp = s;
    temp.toUpperCase();
    bool active{false};
    active = (s == "TRUE" );
    if (!active && (s.toInt() > 0)) { active=true; }
    return active;
  }

  void formatBinary8(String& str, uint8_t num){
    char s[9];
    s[9]='\0';
    for (uint8_t i=0; i<8; ++i){
      s[i] = ((num>>(7-i) & 1) == 0) ? '0':'1';
    }
    str=s;
  }

};