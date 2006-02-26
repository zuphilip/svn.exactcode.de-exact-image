
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
  
  /* convert to RGB color-space - we do not handle palet images internally */
  if (!clr_tbl || *spp >= 3)
    return data;
  
  // detect correct 1bps b/w tables ?
  if (*bps == 1) {
    if (clr_tbl[0] == 0 && clr_tbl[1] == 0 && clr_tbl[2] == 0 &&
	clr_tbl[3] == 255 && clr_tbl[4] == 255 && clr_tbl[5] == 255)
      {
	std::cerr << "correct b/w table." << std::endl;
	return data;
      }
  }
  
  // detect gray tables
  bool is_gray = false;
  if (*spp < 3) {
    int i;
    std::cerr << "checking for gray table." << std::endl;
    for (i = 0; i < clr_tbl_size; ++i) {
      if (clr_tbl[i*clr_tbl_elems+0] != clr_tbl[i*clr_tbl_elems+1] ||
	  clr_tbl[i*clr_tbl_elems+0] != clr_tbl[i*clr_tbl_elems+2])
	break;
    }
    if (i == clr_tbl_size) {
      std::cerr << "correct gray table." << std::endl;
      is_gray = true;
    }
    
    // TODO: check if this optimization is legal
    if (is_gray && *bps == 8 && clr_tbl_size == 256)
      return data;
  }
  
  /*if (is_gray) {
    std::cerr << "TODO: convert to GRAY" << std::endl;
  }
  else*/ {
    std::cerr << "TODO: convert to RGB" << std::endl;
    unsigned char* orig_data = data;
    data = (unsigned char*)malloc (*w * *h * 3);
    
    unsigned char* src = orig_data;
    unsigned char* dst = data;
    
    int bits_used = 0;
    char bit_mask = 0xFF >> (8 - *bps);
    while (dst < data + *w * *h * 3)
      {
	unsigned char v = *src & bit_mask;
	// BMP stores the color table as BGR
	*dst++ = clr_tbl[v*clr_tbl_elems+2];
	*dst++ = clr_tbl[v*clr_tbl_elems+1];
	*dst++ = clr_tbl[v*clr_tbl_elems+0];
	
	bits_used += *bps;
	if (bits_used == 8) {
	  ++src;
	  bits_used = 0;
	}
	else {
	  *src >>= *bps;
	  bits_used += *bps;
	}
      }
    free (orig_data);
    
    *spp = 3;
    *bps = 8;
  }
  return data;
}

void
write_BMP_file (const char* file, unsigned char* data, int w, int h,
		int bps, int spp, int xres, int yres)
{
  // return write_bmp (file, w, h, bps, spp, xres, yres);
}
