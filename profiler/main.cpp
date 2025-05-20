#include "../shared/my_std.h"

#include <intrin.h>
#include <stdio.h>
#include <math.h>

#include "../shared/perf_metrics.h"
#define PERF_PROFILER_DISABLE 1
#include "../shared/perf_profiler.h"


void wow(int i){
  PerfProfilerFunction();

  for(int i = 0; i < 1000000; i++){
    // do nothing
  }
  for(int i = 0; i < 1000000; i++){
    PerfProfilerBlock("Inner Loop 2");
    // do nothing
  }
  for(int i = 0; i < 3000000; i++){
    // do nothing
  }
  if(i > 0)
    wow(i - 1);

}
u64 fib(u64 i){
  PerfProfilerFunction();
  if(i == 39) {
    PerfProfilerBlock("Fib 39");
    wow(20);
  }
  if(i < 2)return i;
  return fib(i - 1) + fib(i - 2);
}

int main(int argc, char **argv){

  printf("Starting...\n");
  printf("OS Frequency: %llu\n", perf_get_os_counter_frequency());
  auto cpu_freq = perf_compute_cpu_counter_frequency();
  printf("CPU Frequency: %llu\n", cpu_freq);

  PerfProfilerInitialize();

  u64 count = 100000000;
  f64 y = 0;
  {
    PerfProfilerBlock("Shit");
    for(u64 i = 0; i < count; i++){
      PerfProfilerBlock("Outer Loop");
      // do nothing
      y += sqrt(y) + pow(count / 3432.0 * i + 1561.0, 1.1);
    }
    // PerfProfilerBlockSetHitCount(count);
  }
  printf("y = %f\n", y);


  u64 x = 40;
  printf("fib(%llu) = %llu\n", x, fib(x));

  PerfProfilerFinalize();

  return 0;
}

