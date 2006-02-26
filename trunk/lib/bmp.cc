
#include <iostream>

#include "bmp.h"
#include "bmp.hh"

unsigned char*
read_BMP_file (const char* file, int* w, int* h, int* bps, int* spp,
               int* xres, int* yres)
{
  unsigned char* color_table = 0;
  unsigned char* data = read_bmp (file, w, h, bps, spp,
				  xres, yres, &color_table);
  
  if (color_table != 0 && *spp < 3 && *bps <= 8) {
    /* convert to RGB color-space - we do not handle palet images internally */

    // TODO: detect gray tables
    // TODO: detect correct 1bps b/w tables ?
    
    std::cerr << "TODO: convert to RGB" << std::endl;
  }
  
  return data;
}

void
write_BMP_file (const char* file, unsigned char* data, int w, int h,
		int bps, int spp, int xres, int yres)
{
  // return write_bmp (file, w, h, bps, spp, xres, yres);
}
