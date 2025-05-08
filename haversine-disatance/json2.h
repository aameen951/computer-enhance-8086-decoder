#ifndef JSON2_H_
#define JSON2_H_

#include "my_std.h"

struct Pair {
  f64 x0;
  f64 y0;
  f64 x1;
  f64 y1;
};

int wow(){

  auto file_cursor = json_open(file_contents);

  if(json_current_element_type(file_cursor) == OBJECT) {
    json_begin_object(file_cursor);
    while(json_more_members(file_cursor)) {
      
    }
  }



}

#endif