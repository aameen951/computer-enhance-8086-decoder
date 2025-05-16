#ifndef HAVERSINE_H
#define HAVERSINE_H

#include "my_std.h"
#include <math.h>

#define EARTH_RADIUS 6371.0 // Earth radius in km

static inline f64 to_radians(f64 degrees) {
  return (PI / 180.0) * degrees;
}
static inline f64 sqr(f64 v){ return v * v; }
// static inline f64 sqrt(f64 v){ return sqrt(v); }

static inline f64 haversine(f64 lat1, f64 lon1, f64 lat2, f64 lon2, f64 earth_radius) {

  lat1 = to_radians(lat1);
  lon1 = to_radians(lon1);
  lat2 = to_radians(lat2);
  lon2 = to_radians(lon2);

  auto d_lat = (lat2 - lat1);
  auto d_lon = (lon2 - lon1);

  auto hav = sqr(sin(d_lat / 2)) + cos(lat1) * cos(lat2) * sqr(sin(d_lon / 2));

  auto res = 2.0 * earth_radius * asin(sqrt(hav));

  return res;
}

#endif // HAVERSINE_H