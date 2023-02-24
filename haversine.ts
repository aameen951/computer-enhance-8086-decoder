import fs from 'fs';

function haversine_degrees(x0: number, y0: number, x1: number, y1: number, r: number) {
  const dy = (y1 - y0) / 180 * Math.PI;
  const dx = (x1 - x0) / 180 * Math.PI;
  y0 = (y0) / 180 * Math.PI;
  y1 = (y1) / 180 * Math.PI;

  const root = Math.pow(Math.sin(dy/2), 2) + Math.cos(y0)*Math.cos(y1)*Math.pow(Math.sin(dx/2), 2)
  const result = 2 * r * Math.asin(Math.sqrt(root));
  return result;
}

let start_time = process.hrtime.bigint();
let result = fs.readFileSync(`./output.json`, {encoding: 'utf-8'}); 
let after_read = process.hrtime.bigint();

let read_time = after_read - start_time;

const data1 = JSON.parse(result).pairs;
const data = data1;//[...data1, ...data1, ...data1, ...data1, ...data1, ...data1, ...data1, ...data1, ...data1, ...data1];
let after_parse = process.hrtime.bigint();

let parse_time = after_parse - after_read;

let checksum = 0;
for(let p of data) {
  checksum += p.x0 + p.y0 - p.x1 - p.y1;
}
let after_compute_checksum = process.hrtime.bigint();
let checksum_time = after_compute_checksum - after_parse;

const EARTH_RADIUS_KM = 6371;
let sum = 0;
let count = 0;
for(let p of data) {
  sum += haversine_degrees(p.x0, p.y0, p.x1, p.y1, EARTH_RADIUS_KM);
  count ++;
}
let after_haversine = process.hrtime.bigint();

let haversine_time = after_haversine - after_compute_checksum;


let end_time = after_haversine;
let total_time = parse_time + checksum_time + haversine_time;

console.log("\n*** JavaScript Implementation ***");
console.log("\nCOORDINATE COUNT:", count);
console.log("DISTANCE AVERAGE:", sum / count);
console.log("CHECKSUM:", checksum, "\n");



console.log(`Read time: ${Number(read_time)/1000000000}sec`);
console.log(`JSON parse time: ${Number(parse_time)/1000000000}sec`);
console.log(`Compute checksum time: ${Number(checksum_time)/1000000000}sec`);
console.log(`Compute haversine time: ${Number(haversine_time)/1000000000}sec`);
console.log(`Total: ${Number(total_time)/1000000000}sec`);
console.log(`Throughput: ${count/(Number(total_time)/1000000000)} haversine/second`);
