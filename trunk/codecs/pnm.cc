
/*
 * C++ PNM library.
 * Copyright (c) 2006 Rene Rebe <rene@exactcode.de>
 *
 * I former times we used to use the netbpm library here, but as
 * it can not handle in memory or otherwise transferred files, allowing
 * access via the C FILE* exclusively (???!!!) we had to write our
 * own parser here ...
 *
 */

#include <iostream>

#include "pnm.hh"

bool PNMCodec::readImage (std::istream* stream, Image& image)
{
#if 0
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
#endif
  return true;
}

bool PNMCodec::writeImage (std::ostream* stream, Image& image, int quality,
			   const std::string& compress)
{
  // ok writing should be easy ,-) just dump the header
  // and the data thereafter ,-)
  
  int format = 0;
  
  if (image.spp == 1 && image.bps == 1)
    format = 1;
  else if (image.spp == 1)
    format = 2;
  else if (image.spp == 3)
    format = 3;
  else {
    std::cerr << "Not (yet?) supported PBM format." << std::endl;
    return false;
  }
  
  std::string c (compress);
  std::transform (c.begin(), c.end(), c.begin(), tolower);
  
  if (compress != "ascii")
    format += 3;
  
  *stream << "P" << format << std::endl;
  *stream << "# written by ExactImage - report bugs to: <rene@exactcode.de>" << std::endl;

  *stream << image.w << " " << image.h << std::endl;
  
  // maxval
  int maxval = (1 << image.bps) - 1;
  std::cerr << "Maxval: " << maxval << std::endl;
  
  if (image.bps > 1)
    *stream << maxval << std::endl;
  
  Image::iterator it = image.begin ();
  if (compress == "ascii")
    {
      for (int y = 0; y < image.h; ++y)
	{
	  for (int x = 0; x < image.w; ++x)
	    {
	      *it;
	      
	      if (x != 0)
		*stream << " ";
	      
	      if (image.spp == 1) {
		*stream << it.getL() / (255 / maxval);
	      }
	      else {
		uint16_t r, g, b;
		it.getRGB (&r, &g, &b);
		*stream << (int)r << " " << (int)g << " " << (int)b;
	      }
	      ++it;
	    }
	  *stream << std::endl;
	}
    }
  else
    {
    }
  
  return true;
}

PNMCodec pnm_loader;
