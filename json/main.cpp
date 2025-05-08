#include "../shared/my_std.h"
#include "../shared/file_reader.h"
#include "json.h"
#include "../shared/high_res_timer.h"

#include <stdio.h>

u8 DIGIT=0b1, NON_ZERO_DIGIT=0b10, HEX_DIGIT=0b100, EXPONENT_CHAR=0b1000, PLUS_MINUS=0b10000, WHITE_SPACE=0b100000;
u8 LEX_TABLE[256] = {
  0,  0,  0,  0,  0,  0,  0,  0,  0,  32, 32, 0,  0,  32, 0,  0,  
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  
  32, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  
  5,  7,  7,  7,  7,  7,  7,  7,  7,  7,  0,  0,  0,  0,  0,  0,  
  0,  4,  4,  4,  4,  12, 4,  0,  0,  0,  0,  0,  0,  0,  0,  0,  
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  
  0,  4,  4,  4,  4,  12, 4,  0,  0,  0,  0,  0,  0,  0,  0,  0,  
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  
};

struct Element;
struct Range {
  char *data;
  um length;
};
struct Member {
  Range name;
  Element *value;
};
struct Object {
  um count;
  Member *first;
};
struct Array {
  um count;
  Element *first;
};
struct Element {
  u8 kind;
  u8 flags;
  Element *next;
  union {
    Object obj;
    Array arr;
    Range str;
    Range num;
    Member mem;
    bool boolean;
  };
};

struct State {
  u8 *data;
  um idx;
  um size;
};

void skip_ws(State *s){
#if 1
  while(s->idx < s->size && (LEX_TABLE[s->data[s->idx]] & WHITE_SPACE)) {
    s->idx++;
  }
#else
  while(s->idx < s->size && 
    (
      s->data[s->idx] == ' ' ||
      s->data[s->idx] == '\n' ||
      s->data[s->idx] == '\r' ||
      s->data[s->idx] == '\t'
    )
  ) {
    s->idx++;
  }
#endif
}

bool number(State *s) {

  auto negative = false;
  if(s->data[s->idx] == '-') {
    s->idx++;
    negative = true;

    if(s->idx >= s->size) {
      // error
      return false;
    }
  }

  if(s->data[s->idx] == '0') {
    s->idx++;
  } else if(s->data[s->idx] >= '1' && s->data[s->idx] <= '9') {
    s->idx++;
    while(s->idx < s->size && s->data[s->idx] >= '0' && s->data[s->idx] <= '9') {
      s->idx++;
    }
  }

  if(s->idx < s->size && s->data[s->idx] == '.') {
    s->idx++;
    if(s->idx >= s->size || s->data[s->idx] < '0' || s->data[s->idx] > '9') {
      /*error*/
      return false;
    }
    s->idx++;
    while(s->idx < s->size && s->data[s->idx] >= '0' && s->data[s->idx] <= '9') {
      s->idx++;
    }
  }

  if(s->idx < s->size && (s->data[s->idx] == 'e' || s->data[s->idx] == 'E')) {
    s->idx++;
    if(s->idx < s->size && (s->data[s->idx] == '+' || s->data[s->idx] == '-')) {
      s->idx++;
    }
    if(s->idx < s->size && s->data[s->idx] >= '0' && s->data[s->idx] <= '9') {
      s->idx++;
      while(s->idx < s->size && s->data[s->idx] >= '0' && s->data[s->idx] <= '9') {
        s->idx++;
      }
    } else {
      /*error*/
      return false;
    }
  }

  return true;
}
bool string(State *s) {
  s->idx++;

  auto start = s->idx;

  while(s->idx < s->size) {

    if(s->data[s->idx] < 0x20) {
      //error
      return false;
    }

    if(s->data[s->idx] == '"') break;
    
    if(s->data[s->idx] == '\\') {
      s->idx++;

      if(s->idx+1 > s->size) { /* error */ return false;}
      else if(s->data[s->idx] == '"') { s->idx++; }
      else if(s->data[s->idx] == '\\'){ s->idx++; }
      else if(s->data[s->idx] == '/') { s->idx++; }
      else if(s->data[s->idx] == 'b') { s->idx++; }
      else if(s->data[s->idx] == 'f') { s->idx++; }
      else if(s->data[s->idx] == 'n') { s->idx++; }
      else if(s->data[s->idx] == 'r') { s->idx++; }
      else if(s->data[s->idx] == 't') { s->idx++; }
      else if(s->data[s->idx] == 'u') {
        s->idx++;

        if(s->idx+4 > s->size) { /* error */ return false;}

        if(!(LEX_TABLE[s->data[s->idx]] & HEX_DIGIT)) {
          /*error*/ return false;
        }
        s->idx++;

        if(!(LEX_TABLE[s->data[s->idx]] & HEX_DIGIT)) {
          /*error*/ return false;
        }
        s->idx++;

        if(!(LEX_TABLE[s->data[s->idx]] & HEX_DIGIT)) {
          /*error*/ return false;
        }
        s->idx++;

        if(!(LEX_TABLE[s->data[s->idx]] & HEX_DIGIT)) {
          /*error*/ return false;
        }
        s->idx++;

      } else {
        // error
        return false;
      }

    } else {
      s->idx++;
    }
  }

  auto end = s->idx;

  if(s->idx >= s->size || s->data[s->idx] != '"') {
    // error
    return false;
  }
  s->idx++;

  return true;
}

bool element(State *s) {
  skip_ws(s);

  if(s->idx >= s->size) {
    // error
    return false;
  }

  switch(s->data[s->idx]) {

    case '{':/*}*/ {
      // object

      auto ID = s->idx;

      s->idx++;
      skip_ws(s);

      um member_count = 0;

      while(s->idx < s->size && s->data[s->idx] != /*{*/'}') {

        if(s->data[s->idx] != '"') {
          // error
          return false;
        }

        if(!string(s))return false;
        skip_ws(s);

        if(s->idx >= s->size || s->data[s->idx] != ':') {
          // error
          return false;
        }

        s->idx++;

        if(!element(s))return false;

        member_count++;

        if(s->idx < s->size && s->data[s->idx] == ',') {
          s->idx++;
          skip_ws(s);
        } else {
          break;
        }
      }

      if(s->idx >= s->size || s->data[s->idx] != /*{*/'}') {
        // error
        return false;
      }

      s->idx++;

    } break;

    case '[': { /*]*/
      // array

      s->idx++;
      skip_ws(s);

      um element_count = 0;

      while(s->idx < s->size && s->data[s->idx] != /*[*/']') {

        if(!element(s))return false;

        element_count++;

        if(s->idx < s->size && s->data[s->idx] == ',') {
          s->idx++;
          skip_ws(s);
        } else {
          break;
        }
      }

      if(s->idx >= s->size || s->data[s->idx] != /*[*/']') {
        // error
        return false;
      }

      s->idx++;

    } break;

    case '"': {
      
      // string
      if(!string(s))return false;

    } break;

    case '-':
    case '0':case '1':case '2':case '3':case '4':
    case '5':case '6':case '7':case '8':case '9': {
      
      // number
      if(!number(s))return false;

    } break;

    case 't': {
      // true
      if(
        s->idx+4 <= s->size &&
        s->data[s->idx+1] == 'r' &&
        s->data[s->idx+2] == 'u' &&
        s->data[s->idx+3] == 'e'
      ) {
        s->idx += 4;
      } else {
        // error
        return false;
      }

    } break;

    case 'f': {
      // false
      if(
        s->idx+5 <= s->size &&
        s->data[s->idx+1] == 'a' &&
        s->data[s->idx+2] == 'l' &&
        s->data[s->idx+3] == 's' &&
        s->data[s->idx+4] == 'e'
      ) {
        s->idx += 5;
      } else {
        // error
        return false;
      }
    } break;

    case 'n': {
      // null
      if(
        s->idx+4 <= s->size &&
        s->data[s->idx+1] == 'u' &&
        s->data[s->idx+2] == 'l' &&
        s->data[s->idx+3] == 'l'
      ) {
        s->idx += 4;
      } else {
        // error
        return false;
      }
    } break;

    default: {
      //error
      return false;
    } break;

  }

  skip_ws(s);
  return true;
}

void json_parse2(um size, u8 *data) {
  State s = {data, 0, size};

  if(element(&s) && s.idx == s.size) {
    // printf("\nYES\n\n");
  } else {
    printf("\nNO %llu\n\n", s.idx);
  }

}

int main(){

  auto read_res = file_read_content_to_memory("F:\\Workspace\\all\\experiments\\computer-enhance\\haversine-disatance\\output.json");
  if(read_res.ok) {
    f64 min_dur = 99999999999;

    for(int i=0; i<15; i++) {
      auto start = hr_timer();
      json_parse2(read_res.size, read_res.data);
      auto end = hr_timer();
      auto dur = hr_timer_to_seconds(end-start);
      if(dur < min_dur)min_dur = dur;
    }

    auto mb = read_res.size/1024.0/1024.0;
    printf("Took: %6.3fs to parse %6.2fmb file  %6.2f mb/s\n", min_dur, mb, mb/min_dur);

    printf("Cool\n");
    return 0;
  } else {
    fprintf(stderr, "Error: failed to read file\n");
    return 1;
  }
}
