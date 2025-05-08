#ifndef RNG_H
#define RNG_H

#include <stdint.h>

// Algorithm Reference: https://burtleburtle.net/bob/rand/smallprng.html

struct Jsf32RngCtx {
  uint32_t a, b, c, d;
};

uint32_t jsf32_next_u32(Jsf32RngCtx *ctx ) {
  auto a = ctx->a;
  auto b = ctx->b;
  auto c = ctx->c;
  auto d = ctx->d;

  #define rot(x,k) (((x)<<(k))|((x)>>(32-(k))))

  auto e = a - rot(b, 27);
  a      = b ^ rot(c, 17);
  b      = c + d;
  c      = d + e;
  d      = e + a;

  #undef rot

  ctx->a = a;
  ctx->b = b;
  ctx->c = c;
  ctx->d = d;

  return d;
}
Jsf32RngCtx jsf32_init(uint32_t seed) {
  Jsf32RngCtx result = {};
  result.a = 0xf1ea5eed;
  result.b = result.c = result.d = seed;

  for(int i=0; i<20; ++i) {
    jsf32_next_u32(&result);
  }
  return result;
}


struct Jsf64RngCtx {
  uint64_t a, b, c, d;
};
uint64_t jsf64_next_u64(Jsf64RngCtx *ctx ) {
  auto a = ctx->a;
  auto b = ctx->b;
  auto c = ctx->c;
  auto d = ctx->d;

  #define rot(x,k) (((x)<<(k))|((x)>>(64-(k))))

  auto e = a - rot(b, 7);
  a      = b ^ rot(c, 13);
  b      = c + rot(d, 37);
  c      = d + e;
  d      = e + a;

  #undef rot

  ctx->a = a;
  ctx->b = b;
  ctx->c = c;
  ctx->d = d;

  return d;
}
Jsf64RngCtx jsf64_init(uint64_t seed) {
  Jsf64RngCtx result = {};
  result.a = 0xf1ea5eed;
  result.b = result.c = result.d = seed;

  for(int i=0; i<20; ++i) {
    jsf64_next_u64(&result);
  }
  return result;
}

#endif // RNG_H