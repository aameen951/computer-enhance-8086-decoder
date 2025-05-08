#ifndef JSON_H_
#define JSON_H_

#include "my_std.h"


enum JsonError {
  JSON_ERROR_NONE,
  JSON_ERROR_INSUFFICIENT_MEMORY,
  JSON_ERROR_UNKNOWN_CHARACTER,
  JSON_ERROR_STRING_NOT_TERMINATED,
  JSON_ERROR_CONTROL_CHAR_IN_STRING,
  JSON_ERROR_UNKNOWN_ESCAPE_CHARACTER_IN_STRING,
  JSON_ERROR_EXPECTING_STRING_MEMBER_NAME,
  JSON_ERROR_EXPECTING_COLON_AFTER_MEMBER_NAME,
  JSON_ERROR_EXPECTING_A_DIGIT_AFTER_EXPONENT,
  JSON_ERROR_EXPECTING_A_DIGIT_AFTER_NEGATIVE_SIGN,
  JSON_ERROR_EXPECTING_JSON_ELEMENT,
  JSON_ERROR_EXPECTING_A_CLOSING_BRACE,
  JSON_ERROR_EXPECTING_A_CLOSING_BRACKET,
  JSON_ERROR_EXPECTING_END_OF_DATA,
  JSON_ERROR_NOT_IMPLEMENTED,
};

struct JsonString {
  u8 *data;
  um len;
};

struct JsonElement;
struct JsonMember {
  JsonString name;
  JsonElement *value;
  JsonMember *next;
};
struct JsonObject {
  JsonMember *first;
  JsonMember *last;
  um length;
};
struct JsonArray {
  JsonElement *first;
  JsonElement *last;
  um length;
};
enum JsonElementKind {
  JSON_ELEMENT_OBJECT,
  JSON_ELEMENT_ARRAY,
  JSON_ELEMENT_STRING,
  JSON_ELEMENT_NUMBER,
  JSON_ELEMENT_BOOLEAN,
  JSON_ELEMENT_NULL,
};
struct JsonElement {
  JsonElementKind kind;
  JsonElement *next_in_array;
  union {
    JsonObject object[1];
    JsonArray array[1];
    JsonString string[1];
    double number;
    b64 boolean;
  };
};

struct JsonResult {
  JsonElement *root;
};
JsonResult json_parse(um size, u8 *data);
void json_free(JsonElement *el);

#endif