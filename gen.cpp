#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define COORD_TO_GENERATE 20000000

void progress(unsigned long long x, unsigned long long max, unsigned long long every){
  if(x == max || x % every == 0)printf("\rProgress: %.1f%%         ", ((double)(x)/max)*100);
}
int main(){

  auto f = fopen("./output.json", "wb");
  if(f) {
    char buffer[4096];
    char *ptr = buffer;

    printf("Generating %d coordinates...\n", COORD_TO_GENERATE);
    
    ptr += sprintf(ptr, "{\"pairs\":[\n");

    for(int x = 0; x<COORD_TO_GENERATE; x++ ){
      ptr += sprintf(ptr, 
        "\t{\"x0\":%f, \"y0\":%f, \"x1\":%f, \"y1\":%f}%s\n", 
        (float)rand()/RAND_MAX * 360 - 180,
        (float)rand()/RAND_MAX * 180 - 90,
        (float)rand()/RAND_MAX * 360 - 180,
        (float)rand()/RAND_MAX * 180 - 90,
        x+1 == COORD_TO_GENERATE ? "" : ","
      );

      if(ptr-buffer > (sizeof(buffer)-256)) {
        fwrite(buffer, 1, ptr-buffer, f);
        ptr = buffer;
      }
      progress(x, COORD_TO_GENERATE, 500100);
    }
    progress(COORD_TO_GENERATE, COORD_TO_GENERATE, 500100);

    ptr += sprintf(ptr, "]}\n");

    if(ptr-buffer) {
      fwrite(buffer, 1, ptr-buffer, f);
      ptr = buffer;
    }

    fclose(f);
  }

  return 0;
}
