
#include <stdlib.h>
extern "C" {
#include <pam.h>
}

#include <iostream>

#include "pnm.hh"

bool PNMCodec::readImage (std::istream* stream, Image& image)
{
  struct pam inpam;
  memset (&inpam, 0, sizeof(inpam));
  pnm_readpaminit(file, &inpam, sizeof(inpam));

  image.h = inpam.height;
  image.w = inpam.width;
  image.spp = inpam.depth;
  image.bps = 1;
  image.xres = image.yres = 0;
  while ( (1<<image.bps) < (int)inpam.maxval)
    ++image.bps;
  
  tuple* tuplerow = pnm_allocpamrow(&inpam);
  
  image.data = (uint8_t*) malloc (image.Stride()*image.h);
  uint8_t* ptr = image.data;
  for (int row = 0; row < inpam.height; ++row) {
    pnm_readpamrow(&inpam, tuplerow);
    for (int col = 0; col < inpam.width; ++col)
      for (int plane = 0; plane < (int)inpam.depth; ++plane) {
	switch (image.bps) {
	case 1:
	case 2:
	case 4:
	  {
	    int per_byte = 8 / image.bps;
	    *ptr = *ptr << image.bps | (uint8_t) tuplerow[col][plane];
	    if (col % per_byte == per_byte - 1)
	    ++ptr;
	  }
	  break;
	case 8:
	  *ptr++ = (uint8_t) tuplerow[col][plane];
	  break;
	case 16:
	  {
	    uint16_t* t = (uint16_t*)ptr;
	    *t = tuplerow[col][plane];
	    ptr += 2;
	  }
	  break;
	}
      }
    // remainder
    if (image.bps < 8) {
      int per_byte = 8 / image.bps;
      int remainder = per_byte - inpam.width % per_byte;
      if (remainder != per_byte) {
	*ptr <<= remainder * image.bps;
	++ptr;
      }
    }
  }
  pnm_freepamrow(tuplerow);
  
  return true;
}

bool PNMCodec::writeImage (std::ostream* stream, Image& image, int quality,
			   const std::string& compress)
{
  struct pam outpam;
  memset (&outpam, 0, sizeof(outpam));
  
  outpam.file = file;
  outpam.size = outpam.len = sizeof (outpam);
  outpam.format = PAM_FORMAT;
  outpam.height = image.h;
  outpam.width = image.w;
  outpam.depth = image.spp;
  outpam.maxval = (1 << image.bps) - 1;
  if (image.spp == 1 && image.bps == 1)
    strcpy (outpam.tuple_type, PAM_PBM_TUPLETYPE);
  else if (image.spp == 1)
    strcpy (outpam.tuple_type, PAM_PGM_TUPLETYPE);
  else if (image.spp == 2)
    strcpy (outpam.tuple_type, PAM_PGM_TUPLETYPE"_ALPHA");
  else if (image.spp == 4)
    strcpy (outpam.tuple_type, PAM_PPM_TUPLETYPE"_ALPHA");
  else
    strcpy (outpam.tuple_type, PAM_PPM_TUPLETYPE);
  
  pnm_writepaminit(&outpam);
  
  tuple* tuplerow = pnm_allocpamrow(&outpam);
  
  uint8_t* ptr = image.data;
  
  for (int row = 0; row < outpam.height; ++row) {
    for (int col = 0; col < outpam.width; ++col) {
      for (int plane = 0; plane < (int)outpam.depth; ++plane) {
	switch (image.bps) {
	case 1:
	case 2:
	case 4:
	  {
	    int per_byte = 8 / image.bps;
	    int subpos = col % per_byte;
	    tuplerow[col][plane] = (*ptr >> 8 - image.bps*(subpos+1))
	      & 0xff >> (8-image.bps);
	    if (subpos == per_byte - 1)
	      ++ptr;
	  }
	  break;
	case 8:
	  tuplerow[col][plane] = *ptr++;
	  break;
	case 16:
	  {
	    uint16_t* t = (uint16_t*)ptr;
	    tuplerow[col][plane] = *t;
	    ptr += 2;
	  }
	  break;
	}
      }
    }
    pnm_writepamrow(&outpam, tuplerow);
    
    // remainder
    if (image.bps < 8) {
      int per_byte = 8 / image.bps;
      int remainder = per_byte - outpam.width % per_byte;
      if (remainder != per_byte) {
	++ptr;
      }
    }
  }
  
  pnm_freepamrow(tuplerow);
  
  return true;
}

PNMCodec pnm_loader;
