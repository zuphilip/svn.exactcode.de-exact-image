
#include <stdio.h>

#include "tiff.h"
#include "jpeg.h"

int main (int argc, char* argv[])
{
  const char* infile = "test.tif";
  
  if (*++argv) {
    infile = *argv;
  }
  
  int w, h, bps, spp;
  unsigned char* data = read_TIFF_file (infile, &w, &h, &bps, &spp);
  if (!data)
  {
    printf ("Error reading TIFF.\n");
    return 1;
  }
  printf ("read TIFF: bps: %d spp: %d\n", bps, spp);
  
  // if not 1-bit optimize
  if (spp != 1 || bps != 1)
    {
      printf ("the image type needs optimzation - to be added\n");
      
      return 1;
    }
  
  // count bits and decide based on that
  
  return 0;
}

