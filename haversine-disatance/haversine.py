from math import radians, sin, cos, sqrt, asin
import time
import json

earth_radius = 6371

def haversine_deg(x0, y0, x1, y1):
  r = earth_radius
  dy = radians(y1 - y0)
  dx = radians(x1 - x0)
  y0 = radians(y0)
  y1 = radians(y1)

  root = (sin(dy/2)**2) + cos(y0)*cos(y1)*(sin(dx/2)**2)
  result = 2 * r * asin(sqrt(root))
  return result


start_time = time.time()

json_file = open('output.json')

after_read = time.time()

read_time = after_read - start_time

json_input = json.load(json_file)['pairs']
# json_input = json_input + json_input + json_input + json_input + json_input + json_input + json_input + json_input + json_input + json_input

after_parse = time.time()
parse_time = after_parse-after_read

checksum = 0
for p in json_input:
  checksum += p['x0'] + p['y0'] - p['x1'] - p['y1'];

after_compute_checksum = time.time()
checksum_time = after_compute_checksum-after_parse

sum = 0
count = 0
for p in json_input:
  sum += haversine_deg(p['x0'], p['y0'], p['x1'], p['y1'])
  count += 1
average = sum / count

after_haversine = time.time()
haversine_time = after_haversine-after_compute_checksum

end_time = after_haversine

total_time = parse_time + checksum_time + haversine_time


print("\n*** Python Implementation ***");
print("\nCOORDINATE COUNT: " + str(count));
print("DISTANCE AVERAGE: " + str(sum / count));
print("CHECKSUM: " + str(checksum) + "\n");

print("Read time: " + str(read_time) + " sec");
print("JSON parse time: " + str(parse_time) + " sec");
print("Compute checksum time: " + str(checksum_time) + " sec");
print("Compute haversine time: " + str(haversine_time) + " sec");
print("Total: " + str(total_time) + " sec");
print("Throughput: " + str(count / (total_time)) + " haversine/second");
