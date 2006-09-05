
/*
 * Bardecode frontend with ExactImage image i/o backend."
 * Copyright (C) 2006 René Rebe for Archivista
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2. A copy of the GNU General
 * Public License can be found in the file LICENSE.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANT-
 * ABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details.
 * 
 */

#include <math.h>

#include <iostream>
#include <iomanip>

#include <algorithm>

#include "config.h"

#include "ArgumentList.hh"

#include "Image.hh"
#include "ImageLoader.hh"

#include "Colorspace.hh"

#include "scale.hh"
#include "rotate.hh"
#include "Matrix.hh"
#include "riemersma.h"
#include "floyd-steinberg.h"

#include <functional>

// barcode library
extern "C" { // missing in the library :-(
#include "barcode.h"

typedef struct tagBITMAP
{
	int bmType;
  int bmWidth;
  int bmHeight;
  int bmWidthBytes;
  unsigned char bmPlanes;
  unsigned char bmBitsPixel;
  void* bmBits;
} BITMAP;

int STReadBarCodeFromBitmap (void *hBarcode, BITMAP *pBitmap, float resolution,
                             char ***bc, char ***bc_type, short photometric);
}

using namespace Utility;

Image image; // the global Image we work on

int main (int argc, char* argv[])
{
  ArgumentList arglist (true); // enable residual gathering
  
  // setup the argument list
  Argument<bool> arg_help ("", "help",
			   "display this help text and exit");
  arglist.Add (&arg_help);
  
  // parse the specified argument list - and maybe output the Usage
  if (!arglist.Read (argc, argv) || arg_help.Get() == true ||
      arglist.Residuals().size() != 1)
    {
      std::cerr << "Bardecode frontend with ExactImage image i/o backend."
		<< std::endl << "Version " VERSION
                <<  " - Copyright (C) 2006 by René Rebe for Archivista" << std::endl
                << "Usage:" << std::endl;
      
      arglist.Usage (std::cerr);
      return 1;
    }
  
  // read the image
  Image image;
  const std::string filename = arglist.Residuals() [0];

  if (!ImageLoader::Read (filename, image)) {
    std::cerr << "Error reading input file." << std::endl;
    return false;
  }

  // the barcode library only supports b/w images
  if (image.bps == 16)
    colorspace_16_to_8 (image);
  // convert any RGB to GRAY
  if (image.spp == 3)
    colorspace_rgb8_to_gray8 (image);

	// testing showed the library does not like 2bps ...
  if (image.bps == 2)
    colorspace_gray2_to_gray4 (image);

  // we have a 1-8 bits per pixel GRAY image, now
  // build custom allocated bitmap to conform to the barcode library constraits
  // (4 byte row allignment ...) - yes, this sucks majorly

  int stride = image.Stride ();
  std::cerr << "Stride: " << stride << std::endl;
  stride += stride % 4 > 0 ? 4 - stride % 4 : 0;
  std::cerr << "Stride: " << stride << std::endl;

#if 0
  uint8_t* bitmap = (uint8_t*) malloc (stride * image.h);
  uint8_t* bitmap_ptr = bitmap;

  Image::iterator it = image.begin (); 
  for (int y = 0; y < image.h; ++y) {
		uint8_t z = 0;
    for (int x = 0; x < image.w; ++x) {
      *it; // dereference for memory access
      uint8_t l = it.getL();
			z <<= 1;
      if (l > 127)
				z |= 1;
			if (x % 8 == 7)
	      *bitmap_ptr++ = z;
			++it;
    }
    int remainder = 8 - image.w % 8;
    if (remainder != 8)
	    *bitmap_ptr = z << remainder;
    bitmap_ptr = bitmap + stride * y;
  }
#endif

  // call into the barcode library
  void* hBarcode = hBarcode = STCreateBarCodeSession ();
  
  uint16 i = 1;
  STSetParameter(hBarcode, ST_READ_CODE39, &i);
	STSetParameter(hBarcode, ST_READ_CODE128, &i);
	STSetParameter(hBarcode, ST_READ_CODE25, &i);
	STSetParameter(hBarcode, ST_READ_EAN13, &i);
	STSetParameter(hBarcode, ST_READ_EAN8, &i);
	STSetParameter(hBarcode, ST_READ_UPCA, &i);
	STSetParameter(hBarcode, ST_READ_UPCE, &i);

  BITMAP bbitmap;
  bbitmap.bmType = 1; // fixed
  bbitmap.bmWidth = image.w;
  bbitmap.bmHeight = image.h;
  bbitmap.bmWidthBytes = image.Stride(); //stride;
  bbitmap.bmPlanes = 1; // fixed
  bbitmap.bmBitsPixel = image.bps; // 1; // check if something else works as well
  bbitmap.bmBits = image.data; // bitmap;

  std::cerr << "w: " << image.w << ", h: " << image.h << ", spp: " << image.spp
            << ", bps: " << image.bps << ", stride: " << image.Stride()
            << std::endl;

  char **bar_codes;
	char **bar_codes_type;

  int photometric = 1; // 0 == photometric min is black, but this appears to be buggy ???
	int	bar_count = STReadBarCodeFromBitmap (hBarcode, &bbitmap, image.xres,
	                                         &bar_codes, &bar_codes_type,
	                                         photometric);

  for (i = 0; i < bar_count; ++i)
	{
		uint32 TopLeftX, TopLeftY, BotRightX, BotRightY ;
		STGetBarCodePos (hBarcode, i, &TopLeftX, &TopLeftY, &BotRightX, &BotRightY);
		printf ("%s\n", bar_codes[i]) ;
	}

  STFreeBarCodeSession (hBarcode);
  return 0;
}
