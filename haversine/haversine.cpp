#include "../shared/my_std.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "../json/json.h"
#include "../shared/my_string.h"
#include "json_dump.h"
#include "../shared/file_reader.h"
#include "../shared/high_res_timer.h"


struct Pair {
  f64 x0;
  f64 y0;
  f64 x1;
  f64 y1;
};

Pair *data;
um data_length;



b32 deserialize_json(JsonElement *root){
  if(root->kind != JSON_ELEMENT_OBJECT) {
    printf("Error: expecting the root to be an object\n");
    return false;
  }

  JsonElement *pairs = null;
   for(auto m = root->object->first; m; m = m->next) {
    if(str_equal(str(m->name.data, m->name.len), str("pairs"))) {
      pairs = m->value;
    }
  }

  if(!pairs) {
    printf("Error: 'pairs' member wasn't found in the root object\n");
    return false;
  }

  if(pairs->kind != JSON_ELEMENT_ARRAY) {
    printf("Error: 'pairs' member isn't an array\n");
    return false;
  }

  data_length = pairs->array->length;
  data = (Pair *)calloc(1, sizeof(Pair) * data_length);

  if(!data) {
    printf("Error: insufficient memory to store the deserialized pairs\n");
    return false;
  }

  int i=0;
  for(auto p = pairs->array->first; p; p = p->next_in_array) {
    f32 x0 = 0;
    f32 y0 = 0;
    f32 x1 = 0;
    f32 y1 = 0;
    int found = 0;

    if(p->kind != JSON_ELEMENT_OBJECT) {
      printf("Error: pair %d is not an object\n", i+1);
      return false;
    }

    for(auto m = p->object->first; m; m = m->next) {

      if(str_equal(str(m->name.data, m->name.len), str("x0"))) {
        if(m->value->kind != JSON_ELEMENT_NUMBER) {
          printf("Error: x0 for pair %d is not a number\n", i+1);
          return false;
        }
        x0 = m->value->number;
        found++;
      } else if(str_equal(str(m->name.data, m->name.len), str("y0"))) {
        if(m->value->kind != JSON_ELEMENT_NUMBER) {
          printf("Error: y0 for pair %d is not a number\n", i+1);
          return false;
        }
        y0 = m->value->number;
        found++;
      } else if(str_equal(str(m->name.data, m->name.len), str("x1"))) {
        if(m->value->kind != JSON_ELEMENT_NUMBER) {
          printf("Error: x1 for pair %d is not a number\n", i+1);
          return false;
        }
        x1 = m->value->number;
        found++;
      } else if(str_equal(str(m->name.data, m->name.len), str("y1"))) {
        if(m->value->kind != JSON_ELEMENT_NUMBER) {
          printf("Error: y1 for pair %d is not a number\n", i+1);
          return false;
        }
        y1 = m->value->number;
        found++;
      }

    }
    if(found != 4) {
      printf("Error: some of the coordinates for pair %d is not provided\n", i+1);
      return false;
    }

    data[i].x0 = x0;
    data[i].y0 = y0;
    data[i].x1 = x1;
    data[i].y1 = y1;
    i++;
  }

  return true;
}


f64 earth_radius = 6371;

f64 radians(f64 deg) {
  return deg / 180.0 * 3.14159265359;
}
f64 haversine_deg(f64 x0, f64 y0, f64 x1, f64 y1) {
  auto r = earth_radius;
  auto dy = radians(y1 - y0);
  auto dx = radians(x1 - x0);
  y0 = radians(y0);
  y1 = radians(y1);

  auto root = pow(sin(dy/2), 2.0) + cos(y0)*cos(y1)*pow(sin(dx/2), 2.0);
  auto result = 2 * r * asin(sqrt(root));
  return result;
}

int main(){

  auto start = hr_timer();
  
  auto read_res = file_read_content_to_memory("./output.json");
  if(!read_res.ok) {
    printf("Error: failed to read the file.\n");
    return 1;
  }
  auto after_read = hr_timer();
  auto read_time = hr_timer_to_seconds(after_read - start);

  auto res = json_parse(read_res.size, read_res.data);
  if(!res.root) {
    printf("Error: parsing failed\n");
    return 2;
  }
  auto after_parse = hr_timer();
  auto parse_time = hr_timer_to_seconds(after_parse - after_read);

  if(!deserialize_json(res.root)) {
    return 3;
  }
  auto after_deserialize = hr_timer();
  auto deserialize_time = hr_timer_to_seconds(after_deserialize - after_parse);

  f64 checksum = 0;
  for(um i=0; i<data_length; i++) {
    auto p = data[i];
    checksum += p.x0 + p.y0 - p.x1 - p.y1;
  }
  auto after_checksum = hr_timer();
  auto checksum_time = hr_timer_to_seconds(after_checksum - after_deserialize);

  f64 sum = 0;
  um count = 0;
  for(um i=0; i<data_length; i++) {
    auto p = data[i];
    sum += haversine_deg(p.x0, p.y0, p.x1, p.y1);
    count += 1;
  }
  f64 average = sum / count;
  auto after_haversine = hr_timer();
  auto haversine_time = hr_timer_to_seconds(after_haversine - after_checksum);

  auto total_time = parse_time + deserialize_time + checksum_time + haversine_time;


  printf("\n*** C++ Implementation ***\n");
  printf("\n");
  printf("COORDINATE COUNT: %llu\n", data_length);
  printf("CHECKSUM: %f\n", checksum);
  printf("DISTANCE AVERAGE: %f\n", average);
  printf("\n");

  printf("Read time: %f sec\n", read_time);
  printf("JSON parse time: %f sec\n", parse_time);
  printf("Deserialization time: %f sec\n", deserialize_time);
  printf("Compute checksum time: %f sec\n", checksum_time);
  printf("Compute haversine time: %f sec\n", haversine_time);
  printf("Total time: %f sec\n", total_time);
  printf("Throughput: %f sec\n", (f64)data_length / total_time);

  // json_dump(res.root);

  return 0;
}
