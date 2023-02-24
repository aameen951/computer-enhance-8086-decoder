#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define COORD_TO_GENERATE 10000000

// TODO: `fwrite`ing every coordinate is stupidly slow!

int main(){

  auto f = fopen("./output.json", "wb");
  if(f) {
    char buffer[256];

    printf("Generating %d coordinates...\n", COORD_TO_GENERATE);
    
    sprintf(buffer, "{\"pairs\":[\n");fwrite(buffer, 1, strlen(buffer), f);

    for(int x = 0; x<COORD_TO_GENERATE; x++ ){
      sprintf(buffer, 
        "\t{\"x0\":%f, \"y0\":%f, \"x1\":%f, \"y1\":%f}%s\n", 
        (float)rand()/RAND_MAX * 360 - 180,
        (float)rand()/RAND_MAX * 180 - 90,
        (float)rand()/RAND_MAX * 360 - 180,
        (float)rand()/RAND_MAX * 180 - 90,
        x+1 == COORD_TO_GENERATE ? "" : ","
      );
      fwrite(buffer, 1, strlen(buffer), f);

      if(x+1 == COORD_TO_GENERATE || x % 10000 == 0)printf("\rProgress: %.1f%%         ", ((double)(x+1)/COORD_TO_GENERATE)*100);
    }
    sprintf(buffer, "]}\n");fwrite(buffer, 1, strlen(buffer), f);

    fclose(f);
  }

  return 0;
}
