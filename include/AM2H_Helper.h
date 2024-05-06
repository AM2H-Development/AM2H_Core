#ifndef AM2H_Helper_h
#define AM2H_Helper_h
#include "Arduino.h"

namespace AM2H_Helper
{
  uint8_t binToInt(const String &binStr);

  template <class T>
  const T parse_hex(const String &string)
  {
    T number{0};
    uint32_t shift{0};
    int len = string.length();
    for (int pos = 0; pos < len; ++pos)
    {
      char c = string.charAt(len - pos - 1);
      if (c >= '0' && c <= '9')
      {
        number += (c - '0') << shift;
      }
      if (c >= 'A' && c <= 'F')
      {
        number += (c - 'A' + 10) << shift;
      }
      if (c >= 'a' && c <= 'f')
      {
        number += (c - 'a' + 10) << shift;
      }
      if (c == 'x')
      {
        break;
      }
      shift += 4;
    }
    return number;
  }

  template <class T>
  const T parse_number(const String &string)
  {
    uint8_t len = string.length();
    uint8_t type{99};
    String temp_string{""};
    uint8_t pos{0};

    for (int p = 0; p < len; ++p)
    {
      const char c = string.charAt(p);
      if (c == ' ' || c == '\'')
      {
        continue;
      }
      if (pos == 0)
      {
        if (c == '0')
        {
          type = 15;
        }
        else if (c >= '1' && c <= '9')
        {
          type = 10;
        }
        else
        {
          type = 0;
          break;
        }
        temp_string += c;
        ++pos;
        continue;
      }
      if (pos == 1)
      {
        if ((c == 'b' || c == 'B') && type == 15)
        {
          type = 2;
        }
        else if ((c == 'x' || c == 'X') && type == 15)
        {
          type = 16;
        }
        else if (c >= '0' && c <= '9' && type == 10)
        {
        }
        else
        {
          type = 0;
          break;
        }
        temp_string += c;
        ++pos;
        continue;
      }
      if (c >= '0' && c <= '1')
      {
        temp_string += c;
        continue;
      }
      if (c >= '2' && c <= '9' && type > 2)
      {
        temp_string += c;
        continue;
      }
      if (c >= 'a' && c <= 'f' && type == 16)
      {
        temp_string += c;
        continue;
      }
      if (c >= 'A' && c <= 'F' && type == 16)
      {
        temp_string += c;
        continue;
      }
      type = 0;
      break;
    }

    if (type == 2)
    {
      return binToInt(temp_string);
    }
    else if (type == 16)
    {
      return parse_hex<T>(temp_string);
    }
    else if (type == 10)
    {
      return temp_string.toInt();
    }

    return 0;
  }

  void parse_location(char *loc, const String parse);
  bool stringToBool(const String s);
  void formatBinary8(String &str, uint8_t num);
};

#endif