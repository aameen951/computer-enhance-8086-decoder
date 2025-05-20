#ifndef PERF_METRICS_H_
#define PERF_METRICS_H_

#include <Windows.h>
#include <intrin.h>

static inline u64 perf_get_os_counter(){
  LARGE_INTEGER val = {};
  QueryPerformanceCounter(&val);
  return val.QuadPart;
}
static inline u64 perf_get_os_counter_frequency(){
  LARGE_INTEGER val = {};
  QueryPerformanceFrequency(&val);
  return val.QuadPart;
}
static inline u64 perf_get_cpu_counter(){
  auto val = __rdtsc();
  return val;
}
static inline u64 perf_compute_cpu_counter_frequency(f64 wait_time_in_seconds = 0.3){
  auto os_wait_count = wait_time_in_seconds * perf_get_os_counter_frequency();

  auto cpu_start = perf_get_cpu_counter();
  auto os_start = perf_get_os_counter();

  auto cpu_end = cpu_start;
  auto os_end = os_start;

  while(true) {
    os_end = perf_get_os_counter();
    cpu_end = perf_get_cpu_counter();
    if((f64)(os_end - os_start) >= os_wait_count) {
      break;
    }
  }
  auto cpu_freq = (cpu_end - cpu_start) / wait_time_in_seconds;

  return cpu_freq;
}




#endif