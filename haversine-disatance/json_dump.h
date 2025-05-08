#ifndef JSON_DUMP_H_
#define JSON_DUMP_H_

#include "json.h"
#include <stdio.h>


#define INDENT(indent) if(new_line){ printf("%*.s", (indent)*4, ""); } else { new_line = true; }

static void json_dump(JsonElement *e, int indent=0){
  auto new_line = false;
  
  if(e->kind == JSON_ELEMENT_OBJECT) {
    INDENT(indent);printf("{\n");
    for(auto p = e->object->first; p; p = p->next) {
      INDENT(indent+1);printf("%.*s: ", (int)p->name.len, p->name.data);
      json_dump(p->value, indent+1);
    }
    INDENT(indent);printf("}\n");
  } else if(e->kind == JSON_ELEMENT_ARRAY) {
    INDENT(indent);printf("[\n");
    for(auto p = e->array->first; p; p = p->next_in_array) {
      INDENT(indent+1);
      json_dump(p, indent+1);
    }
    INDENT(indent);printf("]\n");
  } else if(e->kind == JSON_ELEMENT_NULL) {
    INDENT(indent);printf("null\n");
  } else if(e->kind == JSON_ELEMENT_STRING) {
    INDENT(indent);printf("'%.*s'\n", (int)e->string->len, e->string->data);
  } else if(e->kind == JSON_ELEMENT_NUMBER) {
    INDENT(indent);printf("%f\n", e->number);
  } else if(e->kind == JSON_ELEMENT_BOOLEAN) {
    INDENT(indent);printf("%s\n", e->boolean ? "true" : "false");
  }
}
#undef INDENT

#endif