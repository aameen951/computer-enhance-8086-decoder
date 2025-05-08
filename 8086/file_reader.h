#ifndef FILE_READER_H_
#define FILE_READER_H_

#include "my_std.h"
#include <stdio.h>
#include <stdlib.h>

struct FileReadResult {
  b64 ok;
  um size;
  u8 *data;
};
static FileReadResult file_read_content_to_memory(char *path) {
  auto f = fopen(path, "rb");
  if(!f) return {};

  fseek(f, 0, SEEK_END);
  um size = ftell(f);
  fseek(f, 0, SEEK_SET);

  u8 *data = (u8 *)malloc(size+1);
  if(!data)return {};

  auto read_data = fread(data, size, 1, f);
  if(read_data != 1) {
    free(data);
    return {};
  }

  fclose(f);

  FileReadResult result = {};
  result.ok = true;
  result.size = size;
  result.data = data;
  return result;
}

#endif