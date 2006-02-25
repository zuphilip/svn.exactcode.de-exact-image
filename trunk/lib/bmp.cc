
#include "bmp.h"

#include "bmp.hh"

unsigned char*
read_BMP_file (const char* file, int* w, int* h, int* bps, int* spp,
               int* xres, int* yres)
{
  return read_bmp (file, w, h, bps, spp, xres, yres);
}

void
write_BMP_file (const char* file, unsigned char* data, int w, int h,
		int bps, int spp, int xres, int yres)
{
  // return write_bmp (file, w, h, bps, spp, xres, yres);
}
