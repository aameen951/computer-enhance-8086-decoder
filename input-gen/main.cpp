#include "../shared/my_std.h"

#include <stdio.h>
#include <stdlib.h>

#include "../shared/haversine.h"

#include "../shared/rng.h"
#define PERF_PROFILER_IMPLEMENTATION
#include "../shared/perf_profiler.h"

f64 rand_f64(Jsf64RngCtx *ctx) {
  PerfProfilerFunction();
  u64 v = jsf64_next_u64(ctx);
  return (f64)v / (f64)U64_MAX;
}

int main(int argc, char **argv) {
  PerfProfilerInitialize();

  if (argc < 3) {
    printf("Usage: %s <seed> <number-of-coordinates>\n", argv[0]);
    return 1;
  }

  u64 seed = strtoull(argv[1], null, 10);
  u64 num_of_coords = strtoull(argv[2], null, 10);
  printf("Seed: %llu\n", seed);
  printf("Number of coordinates: %llu\n", num_of_coords);

  auto rng = jsf64_init(seed);
  
  auto f = fopen("output.json", "wb");
  auto af = fopen("answers.f64", "wb");
  if(f) {
    f64 sum = 0.0;
    fprintf(f, "{\"pairs\":[\n");
    {
      PerfProfilerBlock("Generate coordinates");
      for(u64 i=0; i<num_of_coords; i++){
        auto x0 = (rand_f64(&rng) * 360.0) - 180;
        auto y0 = (rand_f64(&rng) * 180.0) - 90;
        auto x1 = (rand_f64(&rng) * 360.0) - 180;
        auto y1 = (rand_f64(&rng) * 180.0) - 90;

        f64 distance = 0.0;
        {
          PerfProfilerBlock("Haversine");
          distance = haversine(x0, y0, x1, y1, EARTH_RADIUS);
          sum += distance;
        }

        {
          PerfProfilerBlock("Answers Write");
          fwrite(&x0, sizeof(f64), 1, af);
          fwrite(&y0, sizeof(f64), 1, af);
          fwrite(&x1, sizeof(f64), 1, af);
          fwrite(&y1, sizeof(f64), 1, af);
          fwrite(&distance, sizeof(f64), 1, af);
        }

        {
          PerfProfilerBlock("JSON Write");
          fprintf(f, "    {\"x0\":%.30f, \"y0\":%.30f, \"x1\":%.30f, \"y1\":%.30f}%s\n", 
            x0, y0, x1, y1, i+1 == num_of_coords ? "" : ",");
        }
      }
      PerfProfilerBlockSetHitCount(num_of_coords);
    }
    fprintf(f, "]}\n");

    auto average = sum / (f64)num_of_coords;
    fwrite(&average, sizeof(f64), 1, af);

    printf("Average distance: %.16f\n", average);
    fclose(f);
  }

  PerfProfilerFinalize();
  return 0;
}
