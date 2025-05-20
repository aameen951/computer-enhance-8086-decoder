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
void _PerfProfilerFinalize(){
  _perf_profiler_block_end(&_perf_profiler_global_state.total);
  auto total_time = _perf_profiler_global_state.points[_PERF_PROFILER_TOTAL_POINT_ID].inc_time;
  printf("Perf Profiler Results:\n");
  printf("=========================\n");
  printf("%-30s  %-11s  %-6s       %-11s  %-6s       %s\n", "Name", "Time Inc.", "%", "Time Exc.", "%", "Hits");

  for(int i = 0; i < PerfProfilerGlobalState::MAX_POINTS; i++){
    auto point = _perf_profiler_global_state.points[i];
    if(point.name){
      auto inc_percent = (f64)point.inc_time / (f64)total_time * 100.0;
      auto exc_percent = (f64)point.exc_time / (f64)total_time * 100.0;
      printf("%-30s  %-11llu  %%%-6.2f      %-11llu  %%%-6.2f      %llu\n", point.name, point.inc_time, inc_percent, point.exc_time, exc_percent, point.hits);
    }
  }
}
#endif // PERF_PROFILER_IMPLEMENTATION

#endif // PERF_PROFILER_H_