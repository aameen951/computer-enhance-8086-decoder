#include <stdio.h>
#include <stdarg.h>
#include "../shared/file_reader.h"
#include "meta.cpp"

int err_print(const char *format, ...){
  va_list args;
  va_start(args, format);
  auto res = vfprintf(stderr, format, args);
  va_end(args);
  return res;
}


bool parse_8086(u8 *data, um size){

  um parsed = 0;
  while(parsed < size) {
    
  }

  return true;
}

int main(int arg_count, char **args){

  if(arg_count == 2) {

    auto path = args[1];
    auto read_res = file_read_content_to_memory(path);
    if(read_res.ok) {
      printf("bits 16\n");
      if(!parse_8086(read_res.data, read_res.size)) {
        err_print("Error: decoding failed\n");
        return 1;
      }
    } else {
      err_print("Error: could not open '%s'\n", path);
      return 1;
    }

  } else {
    err_print("Error: expecting one argument\n");
    err_print("\n");
    err_print("  Usage: main.exe <bin-file>\n");
    err_print("\n");
    return 1;
  }

  return 0;
}
