
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
  printf ("read TIFF: w: %d: h: %d bps: %d spp: %d\n", w, h, bps, spp);
  
  // if not 1-bit optimize
  if (spp != 1 || bps != 1)
    {
      printf ("the image type needs optimzation - to be added\n");
      
      return 1;
    }
  
  // count bits and decide based on that
  
  // create a bit table for fast lookup
  int bits_set[256] = { 0 };
  for (int i = 0; i < 256; i++) {
    int bits = 0;
    for (int j = i; j != 0; j >>= 1) {
      bits += (j & 0x01);
    }
    bits_set[i] = bits;
  }
  
  int stride = (w * bps * spp + 7) / 8;
  
  int margin = 4;
  
  // count pixels
  int pixels = 0;
  for (int row = margin; row < h-margin; row++) {
    for (int x = margin; x < stride - margin; x++) {
      int b = bits_set [ data[stride*row + x] ];
      // it is a bits_set table - and we want tze zeros ...
      pixels += 8-b;
    }
  }
  printf ("the image has %d dark pixels from a total of %d (%f%%)\n",
	  pixels, w*h, (float)pixels/(w*h) );
  
  FILE* f = fopen ("tiff-load.raw", "w+");
  fwrite (data, stride * h, 1, f);
  fclose(f);
  
  return 0;
}
