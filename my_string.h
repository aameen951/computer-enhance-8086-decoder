#ifndef MY_STRING_H_
#define MY_STRING_H_

#include "my_std.h"

struct Str {
  um length;
  u8 *data;
};

inline um c_str_len(const char *s){
  um result = 0;
  while(s[result] != 0)result++;
  return result;
}

inline Str str() {
  return {};
}
inline Str str(u8 *data, um length) {
  return {length, data};
}
inline Str str(char *c_str) {
  return {c_str_len(c_str), (u8 *)c_str};
}




inline b32 str_equal(Str a, Str b){
  if(a.length != b.length)return false;
  for(um i=0; i<a.length; i++) {
    if(a.data[i] != b.data[i])return false;
  }
  return true;
}

#endif