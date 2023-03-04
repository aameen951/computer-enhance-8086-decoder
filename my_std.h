#ifndef MY_STD_H
#define MY_STD_H

#include <stdint.h>

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

#define null NULL

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define array_count(arr) (sizeof((arr))/sizeof((arr)[0]))

#define B_0000 (0x0)
#define B_0001 (0x1)
#define B_0010 (0x2)
#define B_0011 (0x3)
#define B_0100 (0x4)
#define B_0101 (0x5)
#define B_0110 (0x6)
#define B_0111 (0x7)
#define B_1000 (0x8)
#define B_1001 (0x9)
#define B_1010 (0xA)
#define B_1011 (0xB)
#define B_1100 (0xC)
#define B_1101 (0xD)
#define B_1110 (0xE)
#define B_1111 (0xF)

#define BIN_BYTE(hi, lo) (((hi) << 8) | (lo))

#endif