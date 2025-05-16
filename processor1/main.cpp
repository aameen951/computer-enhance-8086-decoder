#include "../shared/my_std.h"
#include "../shared/file.h"
#include "../shared/haversine.h"

#include <stdio.h>
#include <ctype.h>

struct JsonValue;
enum JsonValueType {
  JSON_NULL,
  JSON_NUMBER,
  JSON_BOOLEAN,
  JSON_STRING,
  JSON_ARRAY,
  JSON_OBJECT,
};
struct JsonObjectEntry {
  String key;
  JsonValue *value;
};
struct JsonValueObject {
  Array<JsonObjectEntry> entries;
};
struct JsonValueArray {
  Array<JsonValue *> elements;
};
struct JsonValue {
  JsonValueType type;
  union {
    double number;
    bool boolean;
    String string;
    JsonValueArray array;
    JsonValueObject object;
  };
};
struct JsonResult {
  bool ok;
  JsonValue *root;
};
void json_free(JsonValue *value);


//////////////////////////////////////////////////

void *_json_alloc(um size) {
  return malloc(size);
}
void _json_skip_ws(u8 **ptr, u8 *end) {
  while(*ptr < end) {
    if(**ptr == ' ' || **ptr == '\n' || **ptr == '\r' || **ptr == '\t') {
      (*ptr)++;
    } else {
      break;
    }
  }
}
#include <assert.h>
#include <string.h>

JsonValue *_json_parse_value(u8 **ptr, u8 *end);

um compute_codepoint_utf8_len(u32 codepoint){
  if(codepoint <= 0x7F) return 1;
  if(codepoint <= 0x7FF) return 2;
  if(codepoint <= 0xFFFF) return 3;
  if(codepoint <= 0x10FFFF) return 4;
  assert(false);
  return 0;
}

#define IS_HEX_DIGIT(c) ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))
#define HEX_TO_DIGIT(c) (u8)(( ((c)>='0'&&(c)<='9') ? (c)-'0' : ((c)>='a'&&(c)<='f') ? (c)-'a'+10 : (c)-'A'+10 ))
bool is_valid_u_escape(u8 *ptr, u8 *end){
  bool result = false;

  if(ptr + 5 < end) {
    auto c0 = ptr[0];
    auto c1 = ptr[1];
    auto c2 = ptr[2];
    auto c3 = ptr[3];
    auto c4 = ptr[4];
    auto c5 = ptr[5];

    if(c0 == '\\' && c1 == 'u') {
      if(
        IS_HEX_DIGIT(c2) &&
        IS_HEX_DIGIT(c3) &&
        IS_HEX_DIGIT(c4) &&
        IS_HEX_DIGIT(c5)
      ) {
        result = true;
      }
    }
  }
  return result;
}
u16 decode_u_escape(u8 *ptr, u8 *end){
  assert(is_valid_u_escape(ptr, end));
  auto c0 = ptr[2];
  auto c1 = ptr[3];
  auto c2 = ptr[4];
  auto c3 = ptr[5];

  auto d0 = HEX_TO_DIGIT(c0);
  auto d1 = HEX_TO_DIGIT(c1);
  auto d2 = HEX_TO_DIGIT(c2);
  auto d3 = HEX_TO_DIGIT(c3);

  auto codepoint = (u16)(d0 << 12 | d1 << 8 | d2 << 4 | d3);
  return codepoint;
}
u32 combine_surrogate_pair(u16 high, u16 low) {
  return 0x10000 + ((high - 0xD800) << 10) + (low - 0xDC00);
}
bool is_valid_high_surrogate(u16 codepoint) {
  return codepoint >= 0xD800 && codepoint <= 0xDBFF;
}
bool is_valid_low_surrogate(u16 codepoint) {
  return codepoint >= 0xDC00 && codepoint <= 0xDFFF;
}
#undef IS_HEX_DIGIT
#undef HEX_TO_DIGIT

bool _json_validate_u_escape(u8 **ptr_p, u8 *end, um *str_len_p) {
  /*
  NOTE: The specification of JSON seems very dump regarding
    \uxxxx escape sequences. It is pretending to be unicode codepoint
    but it is actually UTF-16 which means parsing is going to be
    insane and complicated.
    Basically, we have to check if the 2-byte sequence is a 
    surrogate pair and if it is, parse a second \uxxxx sequence
    to get a proper unicode codepoint.
    Then we can proceed to encode it in target encoding which
    in this case is UTF-8.
  */
  bool result = false;
  auto ptr = *ptr_p;

  if(!is_valid_u_escape(ptr, end)) {
    goto end;
  }

  auto codepoint = decode_u_escape(ptr, end);

  ptr += 6;

  if(is_valid_high_surrogate(codepoint)) {
    // surrogate pair
    if(is_valid_u_escape(ptr, end)) {
      auto codepoint2 = decode_u_escape(ptr, end);
      if(is_valid_low_surrogate(codepoint2)) {
        codepoint = combine_surrogate_pair(codepoint, codepoint2);
        ptr += 6;
      }
    }
  }

  *str_len_p += compute_codepoint_utf8_len(codepoint);

  result = true;

  end:
  *ptr_p = ptr;
  return result;
}
void codepoint_to_utf8(u32 codepoint, u8 **out_p) {
  auto out = *out_p;
  if(codepoint <= 0x7F) {
    *out++ = (u8)codepoint;
  } else if(codepoint <= 0x7FF) {
    *out++ = (u8)(0xC0 | ((codepoint >> 6) & 0x1F));
    *out++ = (u8)(0x80 | (codepoint & 0x3F));
  } else if(codepoint <= 0xFFFF) {
    *out++ = (u8)(0xE0 | ((codepoint >> 12) & 0x0F));
    *out++ = (u8)(0x80 | ((codepoint >> 6) & 0x3F));
    *out++ = (u8)(0x80 | (codepoint & 0x3F));
  } else if(codepoint <= 0x10FFFF) {
    *out++ = (u8)(0xF0 | ((codepoint >> 18) & 0x07));
    *out++ = (u8)(0x80 | ((codepoint >> 12) & 0x3F));
    *out++ = (u8)(0x80 | ((codepoint >> 6) & 0x3F));
    *out++ = (u8)(0x80 | (codepoint & 0x3F));
  }
  *out_p = out;
}
void _json_decode_u_escape(u8 **ptr_p, u8 *end, u8 **out_p) {
  auto ptr = *ptr_p;

  auto codepoint = decode_u_escape(ptr, end);
  ptr += 6;

  if(is_valid_high_surrogate(codepoint)) {
    // surrogate pair
    if(is_valid_u_escape(ptr, end)) {
      auto codepoint2 = decode_u_escape(ptr, end);
      if(is_valid_low_surrogate(codepoint2)) {
        codepoint = combine_surrogate_pair(codepoint, codepoint2);
        ptr += 6;
      }
    }
  }

  codepoint_to_utf8(codepoint, out_p);
  *ptr_p = ptr;
}
#undef IS_HEX_DIGIT

bool _json_parse_raw_string(u8 **ptr_p, u8 *end, String *out) {
  auto result = false;
  auto ptr = *ptr_p;
  assert(ptr < end && *ptr == '"');

  ptr++;
  auto str_start = ptr;
  auto str_length = (u64)0;
  while(ptr < end && *ptr != '"'){
    str_length++;
    if(*ptr < 0x20) goto error;

    if(*ptr == '\\') {
      if(ptr+1 >= end) goto error;
      auto c1 = ptr[1];
      if(c1=='b' || c1=='f' || c1=='n' || c1=='r' || c1=='t' || c1=='"' || c1=='\\' || c1=='/') {
        ptr += 2;
      } else if(c1 == 'u') {
        if(!_json_validate_u_escape(&ptr, end, &str_length)) {
          goto error;
        }
      } else {
        goto error;
      }
    } else {
      ptr += 1;
    }
  }
  auto end_str = ptr;

  if(ptr >= end) goto error;
  assert(*ptr == '"');

  ptr++;

  auto str_data = (u8 *)_json_alloc(str_length + 1);
  if(!str_data) goto error;

  {
    auto p = str_start;
    auto out = str_data;
    while(p < end_str) {
      auto c = *p;
      if(c != '\\') {
        *out = c;
        out++, p++;
      } else {
        c = p[1];
        if(c=='b')*out = '\b', out++, p+=2;
        else if(c=='f')*out = '\f', out++, p+=2;
        else if(c=='n')*out = '\n', out++, p+=2;
        else if(c=='r')*out = '\r', out++, p+=2;
        else if(c=='t')*out = '\t', out++, p+=2;
        else if(c=='"')*out =  '"', out++, p+=2;
        else if(c=='\\')*out ='\\', out++, p+=2;
        else if(c=='/')*out = '/', out++, p+=2;
        else if(c=='u')_json_decode_u_escape(&p, end, &out);
        else {
          assert(false);
        }
      }
    }
    assert(p == end_str);
    assert(str_data + str_length == out);
  }

  str_data[str_length] = 0;
  out->data = (char *)str_data;
  out->length = str_length;

  result = true;

  error:
  *ptr_p = ptr;
  return result;
}
JsonValue *_json_parse_string(u8 **ptr_p, u8 *end) {
  auto result = (JsonValue *)null;

  auto ptr = *ptr_p;
  assert(ptr < end && *ptr == '"');

  String str = {};
  if(!_json_parse_raw_string(&ptr, end, &str)) goto error;

  result = (JsonValue *)_json_alloc(sizeof(JsonValue));
  if(!result) goto error;

  result->type = JSON_STRING;
  result->string.data = str.data;
  result->string.length = str.length;

  error:;
  if(!result) {
    if(str.data)free(str.data);
  }
  *ptr_p = ptr;
  return result;
}
JsonValue *_json_parse_object(u8 **ptr_p, u8 *end) {
  auto result = (JsonValue *)null;
  auto ptr = *ptr_p;
  assert(ptr < end && *ptr == '{');

  ptr++;
  _json_skip_ws(&ptr, end);

  Array<JsonObjectEntry> entries = {};

  while(ptr < end && *ptr != '}') {
    if(*ptr != '"')goto error;

    String key = {};
    if(!_json_parse_raw_string(&ptr, end, &key)) {
      goto error;
    }

    _json_skip_ws(&ptr, end);

    if(ptr >= end || *ptr != ':')goto error;
    ptr++;

    auto value = _json_parse_value(&ptr, end);
    if(!value) {
      if(key.data)free(key.data);
      goto error;
    }

    JsonObjectEntry entry = {};
    entry.key.data = key.data;
    entry.key.length = key.length;
    entry.value = value;
    arr_push(&entries, entry);

    _json_skip_ws(&ptr, end);
    
    if(ptr >= end || *ptr != ',')break;
    ptr++;
    _json_skip_ws(&ptr, end);
  }

  if(ptr >= end || *ptr != '}') goto error;
  ptr++;

  result = (JsonValue *)_json_alloc(sizeof(JsonValue));
  if(result) {
    result->type = JSON_OBJECT;
    result->object.entries = entries;
  }

  error:;
  if(!result) {
    // TODO:
  }
  *ptr_p = ptr;
  return result;
}
JsonValue *_json_parse_array(u8 **ptr_p, u8 *end) {
  auto result = (JsonValue *)null;

  auto ptr = *ptr_p;
  assert(ptr < end && *ptr == '[');

  ptr++;
  _json_skip_ws(&ptr, end);

  Array<JsonValue *> elements = {};

  while(ptr < end && *ptr != ']') {

    auto v = _json_parse_value(&ptr, end);
    if(!v)goto error;

    arr_push(&elements, v);
    
    _json_skip_ws(&ptr, end);

    if(ptr >= end || *ptr != ',')break;
    ptr++;
    _json_skip_ws(&ptr, end);
  }

  if(ptr >= end || *ptr != ']') {
    goto error;
  }
  ptr++;
  
  result = (JsonValue *)_json_alloc(sizeof(JsonValue));
  if(result) {
    result->type = JSON_ARRAY;
    result->array.elements = elements;
  }

  error: ;
  if(!result) {
    for(um i=0; i<elements.length; i++) {
      json_free(elements[i]);
    }
    arr_free(&elements);
  }
  *ptr_p = ptr;
  return result;
}
JsonValue *_json_parse_number(u8 **ptr_p, u8 *end) {
  auto result = (JsonValue *)null;

  auto ptr = *ptr_p;
  assert(ptr < end && (*ptr == '-' || (*ptr >= '0' && *ptr <= '9')));

  auto num_start = ptr;

  if(*ptr == '-')ptr++;

  if(ptr >= end)goto error;

  if(*ptr == '0') {
    ptr++;
  } else if(*ptr >= '1' && *ptr <= '9') {
    while(ptr < end && *ptr >= '0' && *ptr <= '9') {
      ptr++;
    }
  } else {
    goto error;
  }

  if(ptr < end && *ptr == '.') {
    ptr++;
    if(ptr >= end)goto error;
    if(*ptr < '0' || *ptr > '9')goto error;
    ptr++;
    while(ptr < end && *ptr >= '0' && *ptr <= '9') {
      ptr++;
    }
  }

  if(ptr < end && (*ptr == 'e' || *ptr == 'E')) {
    ptr++;
    if(ptr >= end)goto error;
    if(*ptr == '+' || *ptr == '-')ptr++;
    if(ptr >= end)goto error;
    if(*ptr < '0' || *ptr > '9')goto error;
    ptr++;
    while(ptr < end && *ptr >= '0' && *ptr <= '9') {
      ptr++;
    }
  }

  result = (JsonValue *)_json_alloc(sizeof(JsonValue));
  if(result) {
    result->type = JSON_NUMBER;
    result->number = strtod((char *)num_start, null);
  }

  error:;
  *ptr_p = ptr;
  return result;
}
bool _json_parse_identifier(u8 **ptr, u8 *end, char *identifier, um identifier_len) {
  auto id_start = *ptr;
  while(*ptr < end && (**ptr >='a' && **ptr <= 'z')) (*ptr)++;
  auto id_end = *ptr;

  if(id_end - id_start != identifier_len) return false;

  auto cmp = memcmp(id_start, identifier, identifier_len);
  return cmp == 0;
}
JsonValue *_json_parse_value(u8 **ptr, u8 *end) {
  auto result = (JsonValue *)null;
  _json_skip_ws(ptr, end);
  if(*ptr >= end) return null;

  auto c = **ptr;

  if(c == '"') {
    result = _json_parse_string(ptr, end);
  } else if(c == '{') {
    result = _json_parse_object(ptr, end);
  } else if(c == '[') {
    result = _json_parse_array(ptr, end);
  } else if(c == 't') {
    if(_json_parse_identifier(ptr, end, "true", 4)) {
      result = (JsonValue *)_json_alloc(sizeof(JsonValue));
      if(result) {
        result->type = JSON_BOOLEAN;
        result->boolean = true;
      }
    }
  } else if(c == 'f') {
    if(_json_parse_identifier(ptr, end, "false", 5)) {
      result = (JsonValue *)_json_alloc(sizeof(JsonValue));
      if(result) {
        result->type = JSON_BOOLEAN;
        result->boolean = false;
      }
    }
  } else if(c == 'n') {
    if(_json_parse_identifier(ptr, end, "null", 4)) {
      result = (JsonValue *)_json_alloc(sizeof(JsonValue));
      if(result) {
        result->type = JSON_NULL;
      }
    }
  } else if(c == '-' || (c >= '0' && c <= '9')) {
    result = _json_parse_number(ptr, end);
  }
  
  _json_skip_ws(ptr, end);
  return result;
}

void json_free(JsonValue *value) {
}
JsonResult json_parse(u8 *data, um size) {
  auto ptr = data, end = data + size;
  auto root = _json_parse_value(&ptr, end);
  if(!root) {
    json_free(root);
    root = null;
  }

  JsonResult result = {};
  result.ok = root != null;
  result.root = root;
  return result;
}
JsonValue *json_object_get_key(JsonValueObject obj, char *key){
  for(um i=0; i<obj.entries.length; i++) {
    auto entry = obj.entries[i];
    if(entry.key.length == strlen(key) && memcmp(entry.key.data, key, entry.key.length) == 0) {
      return entry.value;
    }
  }
  return null;
}

int main(int argc, char **argv){

  // Usage: main <input.json> <answers.f64>
  if(argc < 2) {
    printf(" Usage: %s <input.json>\n", argv[0]);
    printf("        %s <input.json> <answers.f64>\n", argv[0]);
    return 1;
  }
  auto input_file_name = argv[1];
  auto answers_file_name = argc == 3 ? argv[2] : null;

  auto input = file_read_content_to_memory(input_file_name);
  if(!input.ok) {
    printf("Error: could not read input file %s\n", input_file_name);
    return 1;
  }

  f64 *answers = null;
  um answers_count = 0;
  f64 answers_sum = 0;

  if(answers_file_name) {
    auto answers_res = file_read_content_to_memory(answers_file_name);
    if(!answers_res.ok) {
      printf("Error: could not read answers file %s\n", answers_file_name);
      return 1;
    }
    if(answers_res.size % sizeof(f64) != 0) {
      printf("Error: answers file size is not a multiple of %zu\n", sizeof(f64));
      return 1;
    }
    auto double_count = answers_res.size / sizeof(f64);
    double_count--;
    if(double_count % 5 != 0) {
      printf("Error: answers file double count must be a multiple of 5 plus one\n");
    }
    answers_count = (double_count / 5);
    answers = (f64 *)answers_res.data;
    answers_sum = answers[(answers_res.size / sizeof(f64)) - 1];
  }

  printf("Input size: %llu\n", input.size);
  
  auto parse_res = json_parse(input.data, input.size);
  if(!parse_res.ok) {
    printf("Error: invalid json when parsing input file %s\n", input_file_name);
    return 1;
  }

  if(parse_res.root->type != JSON_OBJECT) {
    printf("Error: json root must be an object\n");
    return 1;
  }
  auto root = parse_res.root;

  auto pairs = json_object_get_key(root->object, "pairs");
  if(!pairs) {
    printf("Error: json root object must have a 'pairs' key\n");
    return 1;
  }
  if(pairs->type != JSON_ARRAY) {
    printf("Error: json root object 'pairs' key must be an array\n");
    return 1;
  }
  auto pairs_array = pairs->array.elements;
  if(answers && answers_count != pairs_array.length) {
    printf("Error: answers count %llu does not match pairs count %llu\n", answers_count, pairs_array.length);
    return 1;
  }

  auto sum = 0.0;
  for(um i=0; i<pairs_array.length; i++) {
    auto pair = pairs_array[i];
    if(pair->type != JSON_OBJECT) {
      printf("Error: json root object 'pairs' key must be an array of objects\n");
      return 1;
    }
    auto x0 = json_object_get_key(pair->object, "x0");
    auto y0 = json_object_get_key(pair->object, "y0");
    auto x1 = json_object_get_key(pair->object, "x1");
    auto y1 = json_object_get_key(pair->object, "y1");
    if(!x0 || !y0 || !x1 || !y1) {
      printf("Error: json root object 'pairs' key must have 'x0', 'y0', 'x1', and 'y1' keys\n");
      return 1;
    }

    if(x0->type != JSON_NUMBER || y0->type != JSON_NUMBER || x1->type != JSON_NUMBER || y1->type != JSON_NUMBER) {
      printf("Error: json root object 'pairs' key must have 'x0', 'y0', 'x1', and 'y1' keys of type number\n");
      return 1;
    }
    auto x0_value = x0->number;
    auto y0_value = y0->number;
    auto x1_value = x1->number;
    auto y1_value = y1->number;

    auto result = haversine(x0_value, y0_value, x1_value, y1_value, EARTH_RADIUS);
    sum += result;

    if(answers) {
      auto answer_x0 = answers[i*5+0];
      auto answer_y0 = answers[i*5+1];
      auto answer_x1 = answers[i*5+2];
      auto answer_y1 = answers[i*5+3];
      auto answer_hv = answers[i*5+4];
      if(x0_value != answer_x0) {
        printf("Warning: x0     %25.16f does not match answer %25.16f diff: %25.16f\n", x0_value, answer_x0, x0_value-answer_x0);
      }
      if(y0_value != answer_y0) {
        printf("Warning: y0     %25.16f does not match answer %25.16f diff: %25.16f\n", y0_value, answer_y0, y0_value-answer_y0);
      }
      if(x1_value != answer_x1) {
        printf("Warning: x1     %25.16f does not match answer %25.16f diff: %25.16f\n", x1_value, answer_x1, x1_value-answer_x1);
      }
      if(y1_value != answer_y1) {
        printf("Warning: y1     %25.16f does not match answer %25.16f diff: %25.16f\n", y1_value, answer_y1, y1_value-answer_y1);
      }
      if(result != answer_hv) {
        printf("Warning: result %25.16f does not match answer %25.16f diff: %25.16f\n", result, answer_hv, result-answer_hv);
      }
    }
  }

  printf("Pairs count: %llu\n", pairs_array.length);
  auto average = sum / (f64)pairs_array.length;
  printf("Average distance: %f\n", average);

  if(answers && average != answers_sum) {
    printf("Error: average %25.16f does not match answer %25.16f diff: %25.16f\n", average, answers_sum, average-answers_sum);
  }



  return 0;
}