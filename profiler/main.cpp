#include "../shared/my_std.h"

#include <intrin.h>
#include <stdio.h>

#define EXPAND_(a) a 
#define EXPAND(a) EXPAND_(a)
#define CONCAT_(a, b) a##b
#define CONCAT(a, b) CONCAT_(a, b)

struct ProfilePoint {
  const char *name;
  u64 exc_time;
  u64 inc_time;
  u64 hits;
  u64 recursion_index;
};
struct ProfileGlobalState {
  static const uint32_t MAX_POINTS = 4096;
  ProfilePoint points[MAX_POINTS];
  u64 start, end;
};
struct ProfileThreadState {
  uint32_t current_parent;
};

ProfileGlobalState _profile_global_state;
thread_local ProfileThreadState _profile_thread_state;

#define ProfileFunction() ProfileBlock(__func__)
#define ProfileBlock(name) _ProfileBlock_ CONCAT(_block_, __LINE__)(name, __COUNTER__+1)

struct _ProfileBlock_ {
  _ProfileBlock_(const char *name, u32 id) {
    point = &_profile_global_state.points[id];
    point->name = name;
    point->recursion_index += 1;
    
    old_parent = _profile_thread_state.current_parent;
    _profile_thread_state.current_parent = id;
    start = __rdtsc();
  }
  ~_ProfileBlock_() {
    auto end = __rdtsc();
    _profile_thread_state.current_parent = old_parent;
    auto time = end - start;
    _profile_global_state.points[_profile_thread_state.current_parent].exc_time -= time;
    point->recursion_index -= 1;
    point->hits += 1;
    point->exc_time += time;
    if(point->recursion_index == 0) {
      point->inc_time += time;
    }
  }
  u32 old_parent;
  uint64_t start;
  ProfilePoint *point;
};

void ProfileInitialize(){
  _profile_global_state.start = 0;
  _profile_global_state.end = 0;
  for(int i = 0; i < 4096; i++){
    _profile_global_state.points[i].name = nullptr;
    _profile_global_state.points[i].exc_time = 0;
    _profile_global_state.points[i].inc_time = 0;
    _profile_global_state.points[i].hits = 0;
    _profile_global_state.points[i].recursion_index = 0;
  }
  _profile_global_state.start = __rdtsc();
}
void ProfileFinalize(){
  _profile_global_state.end = __rdtsc();

  auto total_time = _profile_global_state.end - _profile_global_state.start;
  printf("Total time: %llu cycles\n", total_time);
  for(int i = 0; i < 4096; i++){
    auto point = _profile_global_state.points[i];
    if(point.name){
      auto inc_percent = (f64)point.inc_time / (f64)total_time * 100.0;
      auto exc_percent = (f64)point.exc_time / (f64)total_time * 100.0;
      printf("%-20s %20llu cycles %6.2f%%   %20llu cycles %6.2f%%   %llu hits\n", point.name, point.inc_time, inc_percent, point.exc_time, exc_percent, point.hits);
    }
  }
}


void wow(int i){
  ProfileFunction();

  for(int i = 0; i < 1000000; i++){
    // do nothing
  }
  for(int i = 0; i < 1000000; i++){
    // ProfileBlock("Inner Loop 2");
    // do nothing
  }
  for(int i = 0; i < 3000000; i++){
    // do nothing
  }
  if(i > 0)
    wow(i - 1);

}

int main(int argc, char **argv){

  ProfileInitialize();

  // for(int i = 0; i < 1000000; i++){
  //   // do nothing
  // }

  wow(10);

  ProfileFinalize();

  return 0;
}

