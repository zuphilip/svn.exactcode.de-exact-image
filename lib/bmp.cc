
#include <iostream>

#include "bmp.h"
#include "bmp.hh"

unsigned char*
read_BMP_file (const char* file, int* w, int* h, int* bps, int* spp,
               int* xres, int* yres)
{
  unsigned char* clr_tbl = 0;
  int clr_tbl_size = 0, clr_tbl_elems = 0;
  
  unsigned char* data = read_bmp (file, w, h, bps, spp,
				  xres, yres, &clr_tbl,
				  &clr_tbl_size, &clr_tbl_elems);
  
  // convert to RGB color-space - we do not handle palet images internally
  
  // no color table anyway or RGB* ?
  if (!clr_tbl || *spp >= 3)
    return data;
  
  // detect 1bps b/w tables
  if (*bps == 1) {
    if (clr_tbl[0] == 0 &&
	clr_tbl[1] == 0 &&
	clr_tbl[2] == 0 &&
	clr_tbl[clr_tbl_elems+0] == 255 &&
	clr_tbl[clr_tbl_elems+1] == 255 &&
	clr_tbl[clr_tbl_elems+2] == 255)
      {
	std::cerr << "correct b/w table." << std::endl;
	return data;
      }
  }
  
  // detect gray tables
  bool is_gray = false;
  if (*spp < 3) {
    int i;
    for (i = 0; i < clr_tbl_size; ++i) {
      if (clr_tbl[i*clr_tbl_elems+0] != clr_tbl[i*clr_tbl_elems+1] ||
	  clr_tbl[i*clr_tbl_elems+0] != clr_tbl[i*clr_tbl_elems+2])
	break;
    }
    if (i == clr_tbl_size) {
      std::cerr << "found gray table." << std::endl;
      is_gray = true;
    }
    
    // TODO: check if this optimization is legal
    if (is_gray && *bps == 8 && clr_tbl_size == 256)
      return data;
  }
  
  int new_size = *w * *h;
  if (!is_gray) // RGB
    new_size *= 3;
  
  unsigned char* orig_data = data;
  data = (unsigned char*) malloc (new_size);
  
  unsigned char* src = orig_data;
  unsigned char* dst = data;
  
  int bits_used = 0;
  int x = 0;
  while (dst < data + new_size)
    {
      unsigned char v = *src >> (8 - *bps);
      if (is_gray) {
	*dst++ = clr_tbl[v*clr_tbl_elems+0];
      } else {
	// BMP stores the color table in BGR order
	*dst++ = clr_tbl[v*clr_tbl_elems+2];
	*dst++ = clr_tbl[v*clr_tbl_elems+1];
	*dst++ = clr_tbl[v*clr_tbl_elems+0];
      }
      
      bits_used += *bps;
      ++x;

      if (bits_used == 8 || x == *w) {
	++src;
	bits_used = 0;
	if (x == *w)
	  x = 0;
      }
      else {
	*src <<= *bps;
      }
    }
  free (orig_data);
  
  *bps = 8;
  if (is_gray)
    *spp = 1;
  else
    *spp = 3;

  return data;
}

void
write_BMP_file (const char* file, unsigned char* data, int w, int h,
		int bps, int spp, int xres, int yres)
{
  // return write_bmp (file, w, h, bps, spp, xres, yres);
}
