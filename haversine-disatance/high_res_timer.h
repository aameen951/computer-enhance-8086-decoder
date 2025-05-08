#ifndef HIGH_RES_TIMER_H_
#define HIGH_RES_TIMER_H_

#include <Windows.h>



inline u64 hr_timer() {
  LARGE_INTEGER v;
  QueryPerformanceCounter(&v);
  return v.QuadPart;
}

f64 hr_timer_to_seconds(u64 delta){
  LARGE_INTEGER f;
  QueryPerformanceFrequency(&f);
  return (f64)delta / f.QuadPart;
}


#endif