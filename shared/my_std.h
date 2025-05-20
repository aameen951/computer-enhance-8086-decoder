#ifndef MY_STD_H
#define MY_STD_H

#include <stdint.h>
#include <wchar.h>
#include <string.h>
#include <stdlib.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;
typedef size_t um;
typedef intptr_t sm;
typedef u8 b8;
typedef u16 b16;
typedef u32 b32;
typedef u64 b64;

typedef float f32;
typedef double f64;

#define U8_MAX  UINT8_MAX
#define U16_MAX UINT16_MAX
#define U32_MAX UINT32_MAX
#define U64_MAX UINT64_MAX

#define S8_MIN  INT8_MIN
#define S16_MIN INT16_MIN
#define S32_MIN INT32_MIN
#define S64_MIN INT64_MIN

#define S8_MAX  INT8_MAX
#define S16_MAX INT16_MAX
#define S32_MAX INT32_MAX
#define S64_MAX INT64_MAX


#define null NULL

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

#define array_count(arr) (sizeof((arr))/sizeof((arr)[0]))
#define arr_count(arr)   (sizeof((arr))/sizeof((arr)[0]))

#define EXPAND_(a) a 
#define EXPAND(a) EXPAND_(a)
#define CONCAT_(a, b) a##b
#define CONCAT(a, b) CONCAT_(a, b)

#define PI 3.14159265358979323846264338327950288419716939937510


#define KILOBYTES(x) ((x) * 1024llu)
#define MEGABYTES(x) (KILOBYTES(x) * 1024llu)
#define GIGABYTES(x) (MEGABYTES(x) * 1024llu)
#define TERABYTES(x) (GIGABYTES(x) * 1024llu)

inline u64 u64_from_hi_lo(u32 hi, u32 lo) {
  return ((u64)hi << 32) | lo;
}

struct String;
struct WString;
struct StringBuffer;
template<typename T> struct Slice;
template<typename T> struct Buffer;
template<typename T> struct Array;

struct String {
  char *data;
  size_t length;
};
struct WString {
  wchar_t *data;
  size_t length;
};
struct StringBuffer {
  char *data;
  size_t length;
  size_t capacity;
};
template<typename T = u8>
struct Buffer {
  T *data;
  size_t size;
  inline T &operator[](size_t i) {
    return data[i];
  }
  inline operator Slice<T>() {
    return {data, size};
  }
};
template<typename T>
struct Slice {
  T *data;
  size_t length;

  inline T &operator[](size_t i) {
    return data[i];
  }
  inline Slice<T> slice(size_t start) {
    return this->slice(start, length);
  }
  inline Slice<T> slice(size_t start, size_t end) {
    return {data + start, end - start};
  }
};
template<typename T>
struct Array {
  T *data;
  size_t length;
  size_t capacity;

  inline T &operator[](size_t i) {
    return data[i];
  }
  inline operator Slice<T>() {
    return {data, length};
  }
  inline operator Slice<u8>() {
    return {(u8 *)data, length*sizeof(T)};
  }
  inline size_t available_room() {
    return capacity - length;
  }
  inline size_t length_in_bytes(){
    return length * sizeof(T);
  }
};

template<typename T>
void buf_set_exact_size(Buffer<T> *buf, size_t new_size){
  buf->data = (T *)realloc(buf->data, new_size * sizeof(T));
  if(new_size > buf->size) {
    memset(buf->data + buf->size, 0, (new_size - buf->size) * sizeof(T));
  }
  buf->size = new_size;
}
template<typename T>
void buf_set_size_at_least(Buffer<T> *buf, size_t new_size){
  auto size = buf->size;
  if(size < new_size) {
    size = 2 * size;
    if(size < 64)size = 64;
    if(size < new_size) {
      size = new_size;
    }
    buf_set_exact_size(buf, size);
  }
}

template<typename T>
void arr_ensure_room_for(Array<T> *arr, size_t room_for){
  if(arr->capacity - arr->length < room_for) {
    auto old_capacity = arr->capacity;
    arr->capacity = 2*arr->capacity;
    if(arr->capacity < 64)arr->capacity = 64;
    if(arr->capacity - arr->length < room_for) {
      arr->capacity = arr->length + room_for + 64;
    }
    arr->data = (T *)realloc(arr->data, arr->capacity * sizeof(T));
    memset(arr->data + old_capacity, 0, (arr->capacity - old_capacity) * sizeof(T));
  }
}
template<typename T>
void arr_set_length(Array<T> *arr, size_t length){
  arr->length = 0;
  arr_ensure_room_for(arr, length);
  arr->length = length;
}
template<typename T>
void arr_push(Array<T> *arr, T &e) {
  arr_ensure_room_for(arr, 1);
  arr->data[arr->length++] = e;
}
template<typename T>
void arr_push(Array<T> *arr, Slice<T> &s) {
  arr_ensure_room_for(arr, s.length);
  memcpy(arr->data + arr->length, s.data, s.length * sizeof(T));
  arr->length += s.length;
}

template<typename T>
void arr_free(Array<T> *arr) {
  if(arr->data)free(arr->data);
  arr->data = NULL;
  arr->length = 0;
  arr->capacity = 0;
}

#define STR(s) String{s, arr_count(s)-1}
#define WSTR(s) WString{s, arr_count(s)/2-1}
inline String str(struct StringBuffer *buf) {
  return {buf->data, buf->length};
}
inline WString wstr(wchar_t *str) {
  return {str, wcslen(str)};
}
inline bool str_equal(String a, String b) {
  if(a.length != b.length)return false;
  return memcmp(a.data, b.data, a.length) == 0;
}
inline void str_buf_ensure_room_for(StringBuffer *buf, size_t room_for){
  if(buf->capacity - buf->length < room_for) {
    auto old_capacity = buf->capacity;
    buf->capacity = 2*buf->capacity;
    if(buf->capacity < 128)buf->capacity = 128;
    if(buf->capacity - buf->length < room_for) {
      buf->capacity = buf->length + room_for + 128;
    }
    buf->data = (char *)realloc(buf->data, buf->capacity);
    memset(buf->data + old_capacity, 0, buf->capacity - old_capacity);
  }
}
inline void str_buf_push(StringBuffer *buf, String s){
  auto len = s.length + 1;
  str_buf_ensure_room_for(buf, len);

  memcpy(buf->data + buf->length, s.data, s.length);
  buf->length += s.length;
  buf->data[buf->length] = 0;
}
inline void str_buf_free(StringBuffer *buf){
  if(buf->data)free(buf->data);
  buf->data = NULL;
  buf->length = 0;
  buf->capacity = 0;
}
inline String str_buf_copy_as_str(StringBuffer *buf){
  String result = {};
  result.data = (char *)malloc(buf->length + 1);
  memcpy(result.data, buf->data, buf->length);
  result.data[buf->length] = 0;
  result.length = buf->length;
  return result;
}
inline StringBuffer str_buf_copy(StringBuffer *buf){
  StringBuffer result = {};
  result.data = (char *)malloc(buf->capacity);
  memcpy(result.data, buf->data, buf->length);
  result.length = buf->length;
  result.capacity = buf->capacity;
  return result;
}
StringBuffer str_buf_printf(char *format, ...);
StringBuffer str_buf_printf_v(char *format, va_list args);
void str_buf_ensure_c_str(StringBuffer *buf);

#ifdef STD_IMPLEMENTATION

StringBuffer str_buf_printf(char *format, ...){
  va_list args;
  va_start(args, format);
  auto result = str_buf_printf_v(format, args);
  va_end(args);
  return result;
}
StringBuffer str_buf_printf_v(char *format, va_list args){
  StringBuffer result = {};

  auto size = vsnprintf(null, 0, format, args);
  if(size > 0) {
    str_buf_ensure_room_for(&result, size + 1);
    vsnprintf(result.data, size + 1, format, args);
    result.length = size;
  }
  return result;
}
void str_buf_ensure_c_str(StringBuffer *buf) {
  str_buf_ensure_room_for(buf, 1);
  for(size_t i=0; i<buf->length; i++) {
    if(buf->data[i] == 0) {
      memcpy(buf->data+i, buf->data+i+1, buf->length-i-1);
      buf->length--;
    }
  }
  buf->data[buf->length] = 0;
}
#endif // STD_IMPLEMENTATION

#endif