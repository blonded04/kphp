#include "runtime/url.h"

#include "common/base64.h"

#include "runtime/array_functions.h"
#include "runtime/regexp.h"

//string f$base64_decode (const string &s) {
//  int result_len = (s.size() + 3) / 4 * 3;
//  string res (result_len, false);
//  result_len = base64_decode (s.c_str(), reinterpret_cast <unsigned char *> (res.buffer()), result_len + 1);
//
//  if (result_len < 0) {
//    return string();
//  }
//
//  res.shrink (result_len);
//  return res;
//}

static const short base64_reverse_table[256] = {
  -2, -2, -2, -2, -2, -2, -2, -2, -2, -1, -1, -2, -2, -1, -2, -2,
  -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
  -1, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, 62, -2, -2, -2, 63,
  52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -2, -2, -2, -2, -2, -2,
  -2,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
  15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -2, -2, -2, -2, -2,
  -2, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
  41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -2, -2, -2, -2, -2,
  -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
  -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
  -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
  -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
  -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
  -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
  -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
  -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2
};

/* Function f$base64_decode ported from https://github.com/php/php-src/blob/master/ext/standard/base64.c#L130
 * "This product includes PHP software, freely available from <http://www.php.net/software/>".
*/
OrFalse <string> f$base64_decode(const string &s, bool strict) {
  /* run through the whole string, converting as we go */
  string::size_type result_len = (s.size() + 3) / 4 * 3;
  string result(result_len, false);
  int i = 0;
  string::size_type j = 0;
  int padding = 0;
  for (size_t pos = 0; pos < s.size(); pos++) {
    int ch = s[pos];
    if (ch == '=') {
      padding++;
      continue;
    }

    ch = base64_reverse_table[ch];
    if (!strict) {
      /* skip unknown characters and whitespace */
      if (ch < 0) {
        continue;
      }
    } else {
      /* skip whitespace */
      if (ch == -1) {
        continue;
      }
      /* fail on bad characters or if any data follows padding */
      if (ch == -2 || padding) {
        return false;
      }
    }

    switch(i % 4) {
      case 0:
        result[j] = (unsigned char)(ch << 2);
        break;
      case 1:
        result[j++] |= ch >> 4;
        result[j] = (unsigned char)((ch & 0x0f) << 4);
        break;
      case 2:
        result[j++] |= ch >> 2;
        result[j] = (unsigned char)((ch & 0x03) << 6);
        break;
      case 3:
        result[j++] |= ch;
        break;
    }
    i++;
  }
  /* fail if the input is truncated (only one char in last group) */
  if (strict && i % 4 == 1) {
    return false;
  }
  /* fail if the padding length is wrong (not VV==, VVV=), but accept zero padding
   * RFC 4648: "In some circumstances, the use of padding [--] is not required" */
  if (strict && padding && (padding > 2 || (i + padding) % 4 != 0)) {
    return false;
  }

  result.shrink (j);

  return result;
}

string f$base64_encode (const string &s) {
  int result_len = (s.size() + 2) / 3 * 4 ;
  string res (result_len, false);
  result_len = base64_encode (reinterpret_cast <const unsigned char *> (s.c_str()), (int)s.size(), res.buffer(), result_len + 1);

  if (result_len != 0) {
    return string();
  }

  return res;
}

ULong f$base64url_decode_ulong (const string &s) {
  unsigned long long result;
  int r = base64url_decode (s.c_str(), reinterpret_cast <unsigned char *> (&result), 8);
  if (r != 8) {
    php_warning ("Can't convert to ULong from base64url string \"%s\"", s.c_str());
    return 0;
  }
  return result;
}

string f$base64url_encode_ulong (ULong val) {
  string res (11, false);
  php_assert (base64url_encode (reinterpret_cast <const unsigned char *> (&val.l), 8, res.buffer(), 12) == 0);

  return res;
}

ULong f$base64url_decode_ulong_NN (const string &s) {
  unsigned char result[8];
  int r = base64url_decode (s.c_str(), result, 8);
  if (r != 8) {
    php_warning ("Can't convert to ULong from base64url string \"%s\"", s.c_str());
    return 0;
  }

  return ((unsigned long long)result[0] << 24) | (result[1] << 16) | (result[2] << 8) | result[3] |
         ((unsigned long long)result[4] << 56) | ((unsigned long long)result[5] << 48) | ((unsigned long long)result[6] << 40) | ((unsigned long long)result[7] << 32);
}

string f$base64url_encode_ulong_NN (ULong val) {
  unsigned long long l = val.l;
  unsigned char str[8];
  str[0] = ((l >> 24) & 255);
  str[1] = ((l >> 16) & 255);
  str[2] = ((l >> 8) & 255);
  str[3] = (l & 255);
  str[4] = (l >> 56);
  str[5] = ((l >> 48) & 255);
  str[6] = ((l >> 40) & 255);
  str[7] = ((l >> 32) & 255);

  string res (11, false);
  php_assert (base64url_encode (str, 8, res.buffer(), 12) == 0);

  return res;
}


static void parse_str_set_array_value (var &arr, const char *left_br_pos, int key_len, const string &value) {
  php_assert (*left_br_pos == '[');
  const char *right_br_pos = (const char *)memchr (left_br_pos + 1, ']', key_len - 1);
  if (right_br_pos != NULL) {
    string next_key (left_br_pos + 1, (dl::size_type)(right_br_pos - left_br_pos - 1));
    if (!arr.is_array()) {
      arr = array <var> ();
    }

    if (next_key.empty()) {
      next_key = string (arr.to_array().get_next_key());
    }

    if (right_br_pos[1] == '[') {
      parse_str_set_array_value (arr[next_key], right_br_pos + 1, (int)(left_br_pos + key_len - (right_br_pos + 1)), value);
    } else {
      arr.set_value (next_key, value);
    }
  } else {
    arr = value;
  }
}

void parse_str_set_value (var &arr, const string &key, const string &value) {
  const char *key_c = key.c_str();
  const char *left_br_pos = (const char *)memchr (key_c, '[', key.size());
   if (left_br_pos != NULL) {
    return parse_str_set_array_value (arr[string (key_c, (dl::size_type)(left_br_pos - key_c))], left_br_pos, (int)(key_c + key.size() - left_br_pos), value);
  }

  arr.set_value (key, value);
}

void f$parse_str (const string &str, var &arr) {
  arr = array <var> ();

  array <string> par = explode ('&', str, INT_MAX);
  int len = par.count();
  for (int i = 0; i < len; i++) {
    const char *cur = par.get_value (i).c_str();
    const char *eq_pos = strchrnul (cur, '=');
    string value;
    if (*eq_pos) {
      value = f$urldecode (string (eq_pos + 1, (dl::size_type)(strlen (eq_pos + 1))));
    }

    string key (cur, (dl::size_type)(eq_pos - cur));
    key = f$urldecode (key);
    parse_str_set_value (arr, key, value);
  }
}

var f$parse_url (const string &s, int component) {
  var result;
  static const char *regexp = "~^(?:([^:/?#]+):)?(?:///?(?:(?:(?:([^:@?#/]+)(?::([^@?#/]+))?)@)?([^/?#:]+)(?::([0-9]+))?))?([^?#]*)(?:\\?([^#]*))?(?:#(.*))?$~";//I'm too lazy
  f$preg_match (string (regexp, (dl::size_type)(strlen (regexp))), s, result);
  if (!result.is_array()) {
    return false;
  }

  swap (result[2], result[4]);
  swap (result[3], result[5]);
  result[3].convert_to_int();

  if ((unsigned int)component < 8u) {
    if (result[component + 1].empty()) {
      return var();
    }
    return result.get_value (component + 1);
  }
  if (component != -1) {
    php_warning ("Wrong parameter component = %d in function parse_url", component);
    return false;
  }

  array <var> res;
  if (!result[1].empty()) {
    res.set_value (string ("scheme", 6), result[1]);
  }
  if (!result[2].empty()) {
    res.set_value (string ("host", 4), result[2]);
  }
  if (!result[3].empty()) {
    res.set_value (string ("port", 4), result[3]);
  }
  if (!result[4].empty()) {
    res.set_value (string ("user", 4), result[4]);
  }
  if (!result[5].to_string().empty()) {
    res.set_value (string ("pass", 4), result[5]);
  }
  if (!result[6].to_string().empty()) {
    res.set_value (string ("path", 4), result[6]);
  }
  if (!result[7].to_string().empty()) {
    res.set_value (string ("query", 5), result[7]);
  }
  if (!result[8].to_string().empty()) {
    res.set_value (string ("fragment", 8), result[8]);
  }

  return res;
}

string f$rawurldecode (const string &s) {
  static_SB.clean().reserve (s.size());
  for (int i = 0; i < (int)s.size(); i++) {
    if (s[i] == '%') {
      int num_high = hex_to_int (s[i + 1]);
      if (num_high < 16) {
        int num_low = hex_to_int (s[i + 2]);
        if (num_low < 16) {
          static_SB.append_char ((num_high << 4) + num_low);
          i += 2;
          continue;
        }
      }
    }
    static_SB.append_char (s[i]);
  }
  return static_SB.str();
}


static const char *good_url_symbols =
    "00000000000000000000000000000000"
    "00000000000001101111111111000000"
    "01111111111111111111111111100001"
    "01111111111111111111111111100000"
    "00000000000000000000000000000000"
    "00000000000000000000000000000000"
    "00000000000000000000000000000000"
    "00000000000000000000000000000000";//[0-9a-zA-Z-_.]

string f$rawurlencode (const string &s) {
  static_SB.clean().reserve (3 * s.size());
  for (int i = 0; i < (int)s.size(); i++) {
    if (good_url_symbols[(unsigned char)s[i]] == '1') {
      static_SB.append_char (s[i]);
    } else {
      static_SB.append_char ('%');
      static_SB.append_char (uhex_digits[(s[i] >> 4) & 15]);
      static_SB.append_char (uhex_digits[s[i] & 15]);
    }
  }
  return static_SB.str();
}

string f$urldecode (const string &s) {
  static_SB.clean().reserve (s.size());
  for (int i = 0; i < (int)s.size(); i++) {
    if (s[i] == '%') {
      int num_high = hex_to_int (s[i + 1]);
      if (num_high < 16) {
        int num_low = hex_to_int (s[i + 2]);
        if (num_low < 16) {
          static_SB.append_char ((num_high << 4) + num_low);
          i += 2;
          continue;
        }
      }
    } else if (s[i] == '+') {
      static_SB.append_char (' ');
      continue;
    }
    static_SB.append_char (s[i]);
  }
  return static_SB.str();
}

string f$urlencode (const string &s) {
  static_SB.clean().reserve (3 * s.size());
  for (int i = 0; i < (int)s.size(); i++) {
    if (good_url_symbols[(unsigned char)s[i]] == '1') {
      static_SB.append_char (s[i]);
    } else if (s[i] == ' ') {
      static_SB.append_char ('+');
    } else {
      static_SB.append_char ('%');
      static_SB.append_char (uhex_digits[(s[i] >> 4) & 15]);
      static_SB.append_char (uhex_digits[s[i] & 15]);
    }
  }
  return static_SB.str();
}
