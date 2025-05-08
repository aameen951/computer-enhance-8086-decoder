import fs from 'fs';

const COORDS_TO_GENERATE = 1000000;
const OUTPUT_PATH = "./output.json";

let _buffer: string[] = [];
let _buffer_size = 0;
fs.writeFileSync(OUTPUT_PATH, "");
function flush(){
  fs.appendFileSync(OUTPUT_PATH, _buffer.join(""));
  _buffer = [];
  _buffer_size = 0;
}
function output(data: string){
  _buffer.push(data);
  _buffer_size += data.length;
  if(_buffer_size > 1 * 1024 * 1024) {
    flush();
  }
}

output('{"pairs":[\n');
let checksum = 0;
for(let x=0; x<COORDS_TO_GENERATE; x++) {
  const x0 = Math.random() * 360 - 180;
  const y0 = Math.random() * 180 - 90;
  const x1 = Math.random() * 360 - 180;
  const y1 = Math.random() * 180 - 90;

  checksum += x0 + y0 - x1 - y1;

  const COMMA = x+1 === COORDS_TO_GENERATE ? "" : ",";
  output(`\t{"x0":${x0.toFixed(6)}, "y0":${y0.toFixed(6)}, "x1":${x1.toFixed(6)}, "y1":${y1.toFixed(6)}}${COMMA}\n`);

  if(x+1 === COORDS_TO_GENERATE || x % 1000 === 0)process.stdout.write(`\r${((x+1) / COORDS_TO_GENERATE*100).toFixed(1)}%          `);
}
output(']}\n');

flush();

console.log("\n\nCHECKSUM:", checksum);
