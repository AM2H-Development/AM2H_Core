#include "AM2H_Helper.h"
#include "AM2H_Datastore.h"

namespace AM2H_Helper
{
  void parse_location(char *loc, const String parse)
  {
    const uint8_t len = (parse.length() >= AM2H_Datastore::LOC_MAX_LEN) ? AM2H_Datastore::LOC_MAX_LEN - 1 : parse.length();
    for (int i = 0; i < len; ++i)
    {
      loc[i] = parse.charAt(i);
    }
    loc[len] = '\0';
  }

  bool stringToBool(const String s)
  {
    String temp = s;
    temp.toUpperCase();
    bool active{false};
    active = (s == "TRUE");
    if (!active && (s.toInt() > 0))
    {
      active = true;
    }
    return active;
  }

  void formatBinary8(String &str, uint8_t num)
  {
    str = "0b.1234567";
    for (uint8_t i = 0; i < 8; ++i)
    {
      str.setCharAt(i + 2, ((num >> (7 - i) & 1) == 0) ? '0' : '1');
    }
  }

  // len 10
  // idx 0123456789
  // str 0b12345678
  uint8_t binToInt(const String &binStr)
  {
    const uint8_t last_idx = binStr.length() - 1;
    constexpr uint8_t first_idx = 2;

    u_int32_t res{0};
    for (uint8_t i = last_idx; i >= first_idx; --i)
    {
      if (binStr.charAt(i) == '1')
      {
        res += 1 << (last_idx-i);
      }
    }
    return static_cast<u_int8_t>(res & 255);
  }

};