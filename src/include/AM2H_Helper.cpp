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
    
};