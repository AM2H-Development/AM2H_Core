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

  String formatBinary8(uint8_t num){
    String s{};
    for (uint8_t i=0; i<8; ++i){
      if ((num & 1) == 0) {
        s="0"+s;
      } else s="1"+s;
      num = num >> 1;
    }
    return s;
  }

};