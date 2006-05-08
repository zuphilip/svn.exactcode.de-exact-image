
#include <stdlib.h>
#include <pam.h>

#include <iostream>

#include "pnm.hh"

bool PNMLoader::readImage (FILE* file, Image& image)
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
  
  image.data = (u_int8_t*) malloc (image.Stride()*image.h);
  u_int8_t* ptr = image.data;
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
	    *ptr = *ptr << image.bps | (u_int8_t) tuplerow[col][plane];
	    if (col % per_byte == per_byte - 1)
	    ++ptr;
	  }
	  break;
	case 8:
	  *ptr++ = (u_int8_t) tuplerow[col][plane];
	  break;
	case 16:
	  {
	    u_int16_t* t = (u_int16_t*)ptr;
	    *t = tuplerow[col][plane];
	    ptr += 2;
	  }
	  break;
	}
      }
  }
  pnm_freepamrow(tuplerow);
  
  return true;
}

bool PNMLoader::writeImage (FILE* file, Image& image)
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
  
  for (int row = 0; row < outpam.height; ++row) {
    for (int column = 0; column < outpam.width; ++column) {
      for (int plane = 0; plane < (int)outpam.depth; ++plane) {
	
      }
    }
    pnm_writepamrow(&outpam, tuplerow);
  }
  
  pnm_freepamrow(tuplerow);
  
  return true;
}

PNMLoader pnm_loader;
