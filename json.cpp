
#include "json.h"
#include <stdlib.h>
#include <assert.h>

void _json_free_member(JsonMember *m){
  if(m->name.data) {
    free(m->name.data);
    m->name.data = NULL;
  }
  json_free(m->value);
  free(m);
}
void json_free(JsonElement *el){
  switch(el->kind) {
    case JSON_ELEMENT_OBJECT: {
      auto n = el->object->first;
      for(um i=0; i<el->array->length; i++) {
        auto p = n;
        n = p->next;
        _json_free_member(p);
      }
      el->object->first = el->object->last = NULL;
    } break;
    case JSON_ELEMENT_ARRAY: {
      auto n = el->array->first;
      for(um i=0; i<el->array->length; i++) {
        auto p = n;
        n = p->next_in_array;
        json_free(p);
      }
      el->array->first = el->array->last = NULL;
    } break;
    case JSON_ELEMENT_STRING: {
      if(el->string->data) {
        free(el->string->data); 
        el->string->data = NULL;
      } 
    }break;
    case JSON_ELEMENT_NUMBER: break;
    case JSON_ELEMENT_BOOLEAN: break;
    case JSON_ELEMENT_NULL: break;
  }
  free(el);
}

void _json_error(ParseState *s, JsonError error) {
  if(s->error == JSON_ERROR_NONE) {
    s->t->kind = JSON_TOKEN_ERROR;
    s->error = error;
  }
}

um _json__compute_string_size(ParseState *s) {
  um result = 0;
  auto p = s->ptr;
  while(p < s->end) {

    if(*p < 0x20) { 
      _json_error(s, JSON_ERROR_CONTROL_CHAR_IN_STRING); 
      return 0;
    }

    if(*p == '"')break;

    if(*p == '\\') {
      p++;
      if     (*p == '"') { result++; p++; }
      else if(*p == '\\'){ result++; p++; }
      else if(*p == '/') { result++; p++; }
      else if(*p == 'b') { result++; p++; }
      else if(*p == 'f') { result++; p++; }
      else if(*p == 'n') { result++; p++; }
      else if(*p == 'r') { result++; p++; }
      else if(*p == 't') { result++; p++; }
      else if(*p == 'u') {
        // TODO: implement HEX escapes
        _json_error(s, JSON_ERROR_NOT_IMPLEMENTED); 
        return 0;
      } else {
        _json_error(s, JSON_ERROR_UNKNOWN_ESCAPE_CHARACTER_IN_STRING); 
        return 0;
      }
    }
    else result++, p++;
  }
  return result;
}
void _json__string(ParseState *s){
  assert(*s->ptr == '"');
  s->ptr++;

  auto size = _json__compute_string_size(s);
  if(s->t->kind == JSON_TOKEN_ERROR) {
    return;
  }

  um string_len = 0;
  auto string = (u8 *)malloc(size + 1);
  if(!string) {
    _json_error(s, JSON_ERROR_INSUFFICIENT_MEMORY);
    return;
  }

  while(s->ptr < s->end) {
    if(*s->ptr < 0x20) { assert(*s->ptr == '"'); return; }

    if(*s->ptr == '"') break;

    if(*s->ptr == '\\') {
      s->ptr++;
      if     (*s->ptr == '"') { string[string_len] = '"';  string_len++; s->ptr++; }
      else if(*s->ptr == '\\'){ string[string_len] = '\\'; string_len++; s->ptr++; }
      else if(*s->ptr == '/') { string[string_len] = '/';  string_len++; s->ptr++; }
      else if(*s->ptr == 'b') { string[string_len] = '\b'; string_len++; s->ptr++; }
      else if(*s->ptr == 'f') { string[string_len] = '\f'; string_len++; s->ptr++; }
      else if(*s->ptr == 'n') { string[string_len] = '\n'; string_len++; s->ptr++; }
      else if(*s->ptr == 'r') { string[string_len] = '\r'; string_len++; s->ptr++; }
      else if(*s->ptr == 't') { string[string_len] = '\t'; string_len++; s->ptr++; }
      else if(*s->ptr == 'u') {
        // todo
        return;
      } else { assert(false); return; }
    }
    else { 
      string[string_len] = *s->ptr;
      string_len++;
      s->ptr++; 
    }
  }
  assert(string_len == size);
  string[string_len] = 0;

  if(*s->ptr != '"') {
    _json_error(s, JSON_ERROR_STRING_NOT_TERMINATED);
    if(string)free(string);
    return;
  }

  s->ptr++;
  s->t->kind = JSON_TOKEN_STRING;
  s->t->string.data = string;
  s->t->string.len = string_len;
}
void _json__number(ParseState *s){

  auto start = s->ptr;

  auto negative = *s->ptr == '-';
  if(negative) {
    s->ptr++;

    if(s->ptr >= s->end) {
      _json_error(s, JSON_ERROR_EXPECTING_A_DIGIT_AFTER_NEGATIVE_SIGN);
      return;
    }
  }

  if(*s->ptr == '0') {
    s->ptr++;
  } else if(*s->ptr >= '1' && *s->ptr <= '9') {
    s->ptr++;
    while(s->ptr < s->end && *s->ptr >= '0' && *s->ptr <= '9') {
      s->ptr++;
    }
  }

  if(s->ptr < s->end && *s->ptr == '.') {
    s->ptr++;
    while(s->ptr < s->end && *s->ptr >= '0' && *s->ptr <= '9') {
      s->ptr++;
    }
  }

  if(s->ptr < s->end && (*s->ptr == 'e' || *s->ptr == 'E')) {
    s->ptr++;
    if(s->ptr < s->end && (*s->ptr == '-' || *s->ptr == '+')) {
      s->ptr++;
    }
    if(s->ptr >= s->end || *s->ptr < '0' || *s->ptr > '9') {
      _json_error(s, JSON_ERROR_EXPECTING_A_DIGIT_AFTER_EXPONENT);
      return;
    } else s->ptr++;
    while(s->ptr < s->end && *s->ptr >= '0' && *s->ptr <= '9') {
      s->ptr++;
    }
  }

  s->t->kind = JSON_TOKEN_NUMBER;
  s->t->number = strtod((char *)start, NULL);
}
void _json_next_token(ParseState *s){
  if(s->ptr >= s->end) {
    s->t->kind = JSON_TOKEN_EOF;
    return;
  }

  switch(*s->ptr) {
    case ' ': case '\n': case '\r': case '\t': { s->ptr++; s->t->kind = JSON_TOKEN_WS; } break;
    case '{': { s->ptr++; s->t->kind = JSON_TOKEN_OPEN_BRACE; } break;
    case '}': { s->ptr++; s->t->kind = JSON_TOKEN_CLOSE_BRACE; } break;
    case '[': { s->ptr++; s->t->kind = JSON_TOKEN_OPEN_BRACKET; } break;
    case ']': { s->ptr++; s->t->kind = JSON_TOKEN_CLOSE_BRACKET; } break;
    case ':': { s->ptr++; s->t->kind = JSON_TOKEN_COLON; } break;
    case ',': { s->ptr++; s->t->kind = JSON_TOKEN_COMMA; } break;
    case '"': _json__string(s); break;

    case '0': 
    case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': 
    case '-': _json__number(s); break;

    default: _json_error(s, JSON_ERROR_UNKNOWN_CHARACTER);
  }
}
void _json_skip_ws(ParseState *s){
  while(s->t->kind == JSON_TOKEN_WS) {
    _json_next_token(s);
  }
}

JsonElement *_json_parse_element(ParseState *s);
JsonElement *_json_parse_object(ParseState *s){
  _json_skip_ws(s);

  auto error = false;

  JsonMember *first = NULL;
  JsonMember *last = NULL;
  um length = 0;

  while(s->t->kind != JSON_TOKEN_CLOSE_BRACE) {
    _json_skip_ws(s);
    
    if(s->t->kind != JSON_TOKEN_STRING) {
      _json_error(s, JSON_ERROR_EXPECTING_STRING_MEMBER_NAME);
      error = true;
      break;
    }

    auto name = s->t->string;
    _json_next_token(s);

    _json_skip_ws(s);

    if(s->t->kind == JSON_TOKEN_COLON) {
      _json_next_token(s);
    } else {
      _json_error(s, JSON_ERROR_EXPECTING_COLON_AFTER_MEMBER_NAME);
      error = true;
      if(name.data)free(name.data);
      break;
    }

    auto value = _json_parse_element(s);
    if(!value) {
      error = true;
      if(name.data)free(name.data);
      break;
    }

    auto m = (JsonMember *)calloc(1, sizeof(JsonMember));
    if(!m) {
      _json_error(s, JSON_ERROR_INSUFFICIENT_MEMORY);
      if(name.data)free(name.data);
      json_free(value);
      error = true;
      break;
    }
    m->name = name;
    m->value = value;

    if(!first)first = last = m;
    else last = last->next = m;
    length++;

    if(s->t->kind == JSON_TOKEN_COMMA) {
      _json_next_token(s);
    } else {
      break;
    }
  }


  if(!error) {
    _json_skip_ws(s);

    auto result = (JsonElement *)calloc(1, sizeof(JsonElement));
    if(result) {
      result->kind = JSON_ELEMENT_OBJECT;
      result->object->first = first;
      result->object->last = last;
      result->object->length = length;
      return result;
    } else {
      _json_error(s, JSON_ERROR_INSUFFICIENT_MEMORY);
      error = true;
    }
  }

  if(error) {
    auto n = first;
    while(n) {
      auto p = n;
      n = p->next;
      _json_free_member(p);
    }
  }

  return NULL;
}
JsonElement *_json_parse_array(ParseState *s){
  _json_skip_ws(s);

  JsonElement *first = NULL;
  JsonElement *last = NULL;
  um length = 0;
  auto error = false;

  while(s->t->kind != JSON_TOKEN_CLOSE_BRACKET) {
    _json_skip_ws(s);
    
    auto el = _json_parse_element(s);
    if(!el) {
      error = true;
      break;
    }

    length++;
    if(!first)first = last = el;
    else last = last->next_in_array = el;

    if(s->t->kind == JSON_TOKEN_COMMA) {
      _json_next_token(s);
    } else {
      break;
    }
  }

  if(!error) {
    _json_skip_ws(s);

    auto result = (JsonElement *)calloc(1, sizeof(JsonElement));
    if(result) {
      result->kind = JSON_ELEMENT_ARRAY;
      result->array->first = first;
      result->array->last = last;
      result->array->length = length;
      return  result;
    } else {
      _json_error(s, JSON_ERROR_INSUFFICIENT_MEMORY);
      error = true;
    }
  }

  if(error) {
    auto n = first;
    while(n) {
      auto p = n;
      n = p->next_in_array;
      json_free(p);
    }
  }

  return NULL;
}


JsonElement *_json_parse_value(ParseState *s){
  JsonElement *result = NULL;

  switch(s->t->kind) {

    case JSON_TOKEN_OPEN_BRACE: {
      _json_next_token(s);

      auto obj = _json_parse_object(s);

      if(s->t->kind != JSON_TOKEN_CLOSE_BRACE) {
        _json_error(s, JSON_ERROR_EXPECTING_A_CLOSING_BRACE);
        json_free(obj);
        return NULL;
      } else {
        _json_next_token(s);
      }
      result = obj;
    } break;
    case JSON_TOKEN_OPEN_BRACKET: {
      _json_next_token(s);

      auto arr = _json_parse_array(s);
      if(s->t->kind != JSON_TOKEN_CLOSE_BRACKET) {
        _json_error(s, JSON_ERROR_EXPECTING_A_CLOSING_BRACKET);
        json_free(arr);
        return NULL;
      } else {
        _json_next_token(s);
      }
      result = arr;

    } break;
    case JSON_TOKEN_STRING: {

      result = (JsonElement *)calloc(1, sizeof(JsonElement));
      if(!result) {
        _json_error(s, JSON_ERROR_INSUFFICIENT_MEMORY);
        return NULL;
      }
      result->kind = JSON_ELEMENT_STRING;
      *result->string = s->t->string;
      _json_next_token(s);

    } break;
    case JSON_TOKEN_NUMBER: {

      result = (JsonElement *)calloc(1, sizeof(JsonElement));
      if(!result) {
        _json_error(s, JSON_ERROR_INSUFFICIENT_MEMORY);
        return NULL;
      }
      result->kind = JSON_ELEMENT_NUMBER;
      result->number = s->t->number;
      _json_next_token(s);

    } break;
    case JSON_TOKEN_TRUE: {
      result = (JsonElement *)calloc(1, sizeof(JsonElement));
      if(!result) {
        _json_error(s, JSON_ERROR_INSUFFICIENT_MEMORY);
        return NULL;
      }
      result->kind = JSON_ELEMENT_BOOLEAN;
      result->boolean = true;
      _json_next_token(s);
    } break;
    case JSON_TOKEN_FALSE: {
      result = (JsonElement *)calloc(1, sizeof(JsonElement));
      if(!result) {
        _json_error(s, JSON_ERROR_INSUFFICIENT_MEMORY);
        return NULL;
      }
      result->kind = JSON_ELEMENT_BOOLEAN;
      result->boolean = false;
      _json_next_token(s);
    } break;
    case JSON_TOKEN_NULL: {
      result = (JsonElement *)calloc(1, sizeof(JsonElement));
      if(!result) {
        _json_error(s, JSON_ERROR_INSUFFICIENT_MEMORY);
        return NULL;
      }
      result->kind = JSON_ELEMENT_NULL;
      _json_next_token(s);
    } break;

    default: {
      _json_error(s, JSON_ERROR_EXPECTING_JSON_ELEMENT);
    } break;
  }

  return result;
}
JsonElement *_json_parse_element(ParseState *s){
  _json_skip_ws(s);
  auto result = _json_parse_value(s);
  _json_skip_ws(s);
  return result;
}

JsonResult json_parse(um size, u8 *data){
  JsonResult result = {};

  ParseState s = {};
  s.data = s.ptr = data;
  s.end = s.data + size;
  _json_next_token(&s);

  auto el = _json_parse_element(&s);

  if(s.t->kind != JSON_TOKEN_EOF) {
    _json_error(&s, JSON_ERROR_EXPECTING_END_OF_DATA);
    return result;
  }

  result.root = el;
  return result;
}

