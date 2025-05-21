#ifndef PERF_PROFILER_H_
#define PERF_PROFILER_H_

#include "my_std.h"
#include "perf_metrics.h"

#ifndef PERF_PROFILER_DISABLE
#define PERF_PROFILER_DISABLE 0
#endif

#define PERF_PROFILER_MAX_POINTS 4096

#if PERF_PROFILER_DISABLE
#  define PerfProfilerInitialize()
#  define PerfProfilerFinalize()
#  define PerfProfilerFunction(...)
#  define PerfProfilerBlock(...)
#  define PerfProfilerBlockSetHitCount(...)
#else
#  define PerfProfilerInitialize() _PerfProfilerInitialize()
#  define PerfProfilerFinalize() _PerfProfilerFinalize()
#  define PerfProfilerFunction() PerfProfilerBlock(__func__)
#  define PerfProfilerBlock(name) PerfProfileBlock CONCAT(_block_, __LINE__)(name, __COUNTER__+_PERF_PROFILER_FIRST_POINT_ID)
#  define PerfProfilerBlockSetHitCount(hit_count) _perf_profiler_global_state.points[_perf_profiler_thread_state.current_parent].hits += (hit_count - 1)
#endif

struct PerfProfilerPoint {
  const char *name;
  u64 exc_time;
  u64 inc_time;
  u64 hits;
  u64 recursion_index;
};
struct PerfProfilerBlockState {
  u32 point_id;
  u32 old_parent;
  u64 start;
};
struct PerfProfilerGlobalState {
  static const uint32_t MAX_POINTS = PERF_PROFILER_MAX_POINTS;
  PerfProfilerPoint points[MAX_POINTS];
  PerfProfilerBlockState total;
};
struct PerfProfilerThreadState {
  uint32_t current_parent;
};

extern PerfProfilerGlobalState _perf_profiler_global_state;
extern thread_local PerfProfilerThreadState _perf_profiler_thread_state;

#define _PERF_PROFILER_TOTAL_POINT_ID 1
#define _PERF_PROFILER_FIRST_POINT_ID (_PERF_PROFILER_TOTAL_POINT_ID + 1)

inline static void _perf_profiler_block_begin(PerfProfilerBlockState *s, const char *name, u32 id) {
  s->point_id = id;
  auto point = &_perf_profiler_global_state.points[s->point_id];
  point->name = name;
  point->recursion_index += 1;
  
  s->old_parent = _perf_profiler_thread_state.current_parent;
  _perf_profiler_thread_state.current_parent = id;
  s->start = perf_get_cpu_counter();
}
inline static void _perf_profiler_block_end(PerfProfilerBlockState *s) {
  auto end = perf_get_cpu_counter();
  _perf_profiler_thread_state.current_parent = s->old_parent;
  auto time = end - s->start;
  _perf_profiler_global_state.points[_perf_profiler_thread_state.current_parent].exc_time -= time;
  auto point = &_perf_profiler_global_state.points[s->point_id];
  point->recursion_index -= 1;
  point->hits += 1;
  point->exc_time += time;
  if(point->recursion_index == 0) {
    point->inc_time += time;
  }
}
struct PerfProfileBlock {
  PerfProfileBlock(const char *name, u32 id) {
    _perf_profiler_block_begin(&s, name, id);
  }
  ~PerfProfileBlock() {
    _perf_profiler_block_end(&s);
  }
  PerfProfilerBlockState s;
};

void _PerfProfilerInitialize();
void _PerfProfilerFinalize();

#ifdef PERF_PROFILER_IMPLEMENTATION

#include <stdio.h>

PerfProfilerGlobalState _perf_profiler_global_state;
thread_local PerfProfilerThreadState _perf_profiler_thread_state;

void _PerfProfilerInitialize(){
  _perf_profiler_block_begin(&_perf_profiler_global_state.total, "Total", _PERF_PROFILER_TOTAL_POINT_ID);

  for(int i = _PERF_PROFILER_FIRST_POINT_ID; i < PerfProfilerGlobalState::MAX_POINTS; i++){
    _perf_profiler_global_state.points[i].name = nullptr;
    _perf_profiler_global_state.points[i].exc_time = 0;
    _perf_profiler_global_state.points[i].inc_time = 0;
    _perf_profiler_global_state.points[i].hits = 0;
    _perf_profiler_global_state.points[i].recursion_index = 0;
  }
}
struct _PerfProfilerTimeWithUnit {
  f64 value;
  const char *unit;
};
static _PerfProfilerTimeWithUnit _perf_profiler_to_time(f64 counter, u64 freq){
  _PerfProfilerTimeWithUnit result = {};
  result.value = (f64)counter / (f64)freq;
  result.unit = "sc";
  if(result.value > 1) {
    if(result.value > 60) {
      result.value /= 60;
      result.unit = "mn";
    }
  } else {
    if(result.value >= 0.001) {
      result.value *= 1000.0;
      result.unit = "ms";
    } else if(result.value >= 0.000001) {
      result.value *= 1000000.0;
      result.unit = "us";
    } else {
      result.value *= 1000000000.0;
      result.unit = "ns";
    }
  }
  return result;
}


void _PerfProfilerFinalize(){
  _perf_profiler_block_end(&_perf_profiler_global_state.total);
  auto c_freq = perf_compute_cpu_counter_frequency();
  auto total_time = _perf_profiler_global_state.points[_PERF_PROFILER_TOTAL_POINT_ID].inc_time;
  printf("\n Perf Profiler Results:\n\n");
  printf("Name                           |             Time Inc.             |             Time Exc.             |     Hits    |        Per-Hit Inc.         |          Per-Hit Exc.       \n");
  printf("=================================================================================================================================================================================\n");

  for(int i = 0; i < PerfProfilerGlobalState::MAX_POINTS; i++){
    auto point = _perf_profiler_global_state.points[i];
    if(point.name){
      auto inc_rtime = _perf_profiler_to_time(point.inc_time, c_freq);
      auto exc_rtime = _perf_profiler_to_time(point.exc_time, c_freq);
      auto inc_percent = (f64)point.inc_time / (f64)total_time * 100.0;
      auto exc_percent = (f64)point.exc_time / (f64)total_time * 100.0;
      auto per_hit_inc = point.hits ? (f64)point.inc_time / (f64)point.hits : 0;
      auto per_hit_exc = point.hits ? (f64)point.exc_time / (f64)point.hits : 0;
      auto per_hit_inc_rtime = _perf_profiler_to_time(per_hit_inc, c_freq);
      auto per_hit_exc_rtime = _perf_profiler_to_time(per_hit_exc, c_freq);

      printf("%-30s | %11llu cy %5.1f %-3s %6.2f %% | %11llu cy %5.1f %-3s %6.2f %% | %11llu | %14.1f cy %5.1f %-3s | %14.1f cy %5.1f %-3s\n", 
        point.name, 
        point.inc_time, inc_rtime.value, inc_rtime.unit, inc_percent, 
        point.exc_time, inc_rtime.value, inc_rtime.unit, exc_percent, 
        point.hits, 
        per_hit_inc, per_hit_inc_rtime.value, per_hit_inc_rtime.unit,
        per_hit_exc, per_hit_exc_rtime.value, per_hit_exc_rtime.unit
      );
    }
  }
  printf("=================================================================================================================================================================================\n\n");
}
#endif // PERF_PROFILER_IMPLEMENTATION

#endif // PERF_PROFILER_H_