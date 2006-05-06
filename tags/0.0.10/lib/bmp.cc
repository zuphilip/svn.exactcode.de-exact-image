
#include <iostream>

#include "bmp.h"
#include "bmp.hh"

#include "Colorspace.hh"

bool BMPLoader::readImage (FILE* file, Image& image)
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
  
  uint16_t* rmap = new uint16_t [1 << image.bps];
  uint16_t* gmap = new uint16_t [1 << image.bps];
  uint16_t* bmap = new uint16_t [1 << image.bps];
  
  for (int i = 0; i < (1 << image.bps); ++i) {
    // BMP maps have BGR order ...
    rmap[i] = clr_tbl[i*clr_tbl_elems+2] << 8;
    gmap[i] = clr_tbl[i*clr_tbl_elems+1] << 8;
    bmap[i] = clr_tbl[i*clr_tbl_elems+0] << 8;
  }
  
  colorspace_de_palette (image, clr_tbl_size, rmap, gmap, bmap);
  
  delete (rmap);
  delete (gmap);
  delete (bmap);
  
  return true;
}

bool BMPLoader::writeImage (FILE* file, Image& image)
{
  // return write_bmp (file, w, h, bps, spp, xres, yres);
  return false;
}

BMPLoader bmp_loader;
