
#include <iostream>

#include "bmp.h"
#include "bmp.hh"

#include "Colorspace.hh"

bool BMPLoader::readImage (const char* file, Image& image)
{
  unsigned char* clr_tbl = 0;
  int clr_tbl_size = 0, clr_tbl_elems = 0;
  
  image.data = read_bmp (file, &image.w, &image.h, &image.bps, &image.spp,
			 &image.xres, &image.yres, &clr_tbl,
			 &clr_tbl_size, &clr_tbl_elems);
  
  // convert to RGB color-space - we do not handle palet images internally
  
  // no color table anyway or RGB* ?
  if (!clr_tbl || image.spp >= 3)
    return true;
  
  // TODO convert to our colormap format
  // colormap_de_palette (image, rmap, gmap, bmap);
  
  return true;
}

bool BMPLoader::writeImage (const char* file, Image& image)
{
  // return write_bmp (file, w, h, bps, spp, xres, yres);
  return false;
}

BMPLoader bmp_loader;
