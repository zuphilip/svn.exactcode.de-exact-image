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
#include "Codecs.hh"

#include "Colorspace.hh"

#include "scale.hh"
#include "rotate.hh"
// #include "Matrix.hh"
#include "riemersma.h"
#include "floyd-steinberg.h"

#include <functional>

// barcode library
extern "C" { // missing in the library header ...
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
  
  // missing in the library header ...
  int STReadBarCodeFromBitmap (void *hBarcode, BITMAP *pBitmap, float resolution,
			       char ***bc, char ***bc_type, short photometric);
}

using namespace Utility;

std::vector<std::string> decodeBarcodes (Image& im, const std::string& codes,
					 int min_length, int max_length)
{
  uint16_t i;
  
  // a copy we can mangle
  Image image;
  
  // yeah - our annoying default copy ownership migration bits again ..
  image = im;
  im.data = image.data;
  image.data = 0;
  image.New (image.w, image.h);
  memcpy (image.data, im.data, image.Stride()*image.h);
  
  // the barcode library does not support such a high bit-depth
  if (image.bps == 16)
    colorspace_16_to_8 (image);
  
  // the library interface only handles one channel data
  if (image.spp == 3)
    colorspace_rgb8_to_gray8 (image);
  
  // the library does not appear to like 2bps ?
  if (image.bps < 8)
    colorspace_grayX_to_gray8 (image);
  
  // now we have a 8 bits per pixel GRAY image
  
  //ImageCodec::Write ("dump.tif", image, 90, "");
  
  // The bardecode library is documented to require a 4 byte row
  // allignment. To conform this a custom allocated bitmap would
  // be required which would either require a complete Image class
  // rewrite or a extremely costly allocation and copy at this
  // location. Depending on the moon this is required or not.
  
  if (true)
    {
      int cur_stride = image.Stride ();
      int stride = cur_stride / 4 * 4;
      
      if (false)
	std::cerr << "Cur Stride: " << cur_stride << std::endl
		  << "New Stride: " << stride << std::endl;
      
      // the new image is definetly smaller, thus we mess with the original data
      // first row stays fixed, thus skip
      for (int y = 1; y < image.h; ++y)
	memmove (image.data + y*stride, image.data + y*cur_stride, stride);
      
      // store new stride == width (@ 8bit gray)
      image.w = stride;
    }

  // call into the barcode library
  void* hBarcode = STCreateBarCodeSession ();
  
  i = 0;
  STSetParameter (hBarcode, ST_READ_CODE39, &i);
  STSetParameter (hBarcode, ST_READ_CODE128, &i);
  STSetParameter (hBarcode, ST_READ_CODE25, &i);
  STSetParameter (hBarcode, ST_READ_EAN13, &i);
  STSetParameter (hBarcode, ST_READ_EAN8, &i);
  STSetParameter (hBarcode, ST_READ_UPCA, &i);
  STSetParameter (hBarcode, ST_READ_UPCE, &i);
  
  // parse the code list
  std::string c (codes);
  std::transform (c.begin(), c.end(), c.begin(), tolower);
  std::string::size_type it = 0;
  std::string::size_type it2;
  i = 1;
  do
    {
      it2 = c.find ('|', it);
      std::string code;
      if (it2 !=std::string::npos) {
	code = c.substr (it, it2-it);
	it = it2 + 1;
      }
      else
	code = c.substr (it);
      
      if (!code.empty())
	{
	  if (code == "code39")
	    STSetParameter(hBarcode, ST_READ_CODE39, &i);
	  else if (code == "code128")
	    STSetParameter(hBarcode, ST_READ_CODE128, &i);
	  else if (code == "code25")
	    STSetParameter(hBarcode, ST_READ_CODE25, &i);
	  else if (code == "ean13")
	    STSetParameter(hBarcode, ST_READ_EAN13, &i);
	  else if (code == "ean8")
	    STSetParameter(hBarcode, ST_READ_EAN8, &i);
	  else if (code == "upca")
	    STSetParameter(hBarcode, ST_READ_UPCA, &i);
	  else if (code == "upce")
	    STSetParameter(hBarcode, ST_READ_UPCE, &i);
	  else
	    std::cerr << "Unrecognized barcode type: " << code << std::endl;
	}
    }
  while (it2 != std::string::npos);
  
  i = min_length;
  STSetParameter (hBarcode, ST_MIN_LEN, &i);
  i = max_length;
  STSetParameter (hBarcode, ST_MAX_LEN, &i);
  
  if (false) // the library claims to have defaults (...)
    {
      i = 15; // all directions
      STSetParameter(hBarcode, ST_ORIENTATION_MASK, &i);
      
      i = 1;
      STSetParameter(hBarcode, ST_MULTIPLE_READ, &i);
      
      i = 20;
      STSetParameter(hBarcode, ST_NOISEREDUCTION, &i);
      i = 1;
      STSetParameter(hBarcode, ST_DESPECKLE, &i);
    }
  
  BITMAP bbitmap;
  bbitmap.bmType = 1; // bitmap type version, fixed v1
  bbitmap.bmWidth = image.w;
  bbitmap.bmHeight = image.h;
  bbitmap.bmWidthBytes = image.Stride();
  bbitmap.bmPlanes = 1; // the library is documented to only take 1
  bbitmap.bmBitsPixel = image.bps; // 1, 4 and 8 appeared to work
  bbitmap.bmBits = image.data; // our class' bitmap data
  
  if (false)
    std::cerr << "@: " << (void*) image.data
	      << ", w: " << image.w << ", h: " << image.h << ", spp: " << image.spp
	      << ", bps: " << image.bps << ", stride: " << image.Stride()
	      << std::endl;
  
  char** bar_codes;
  char** bar_codes_type;
  
  int photometric = 1; // 0 == photometric min is black, but this appears to be buggy ???
  int bar_count = STReadBarCodeFromBitmap (hBarcode, &bbitmap, image.xres,
					   &bar_codes, &bar_codes_type,
					   photometric);
  std::vector<std::string> ret;
  
  for (i = 0; i < bar_count; ++i) {
    uint32 TopLeftX, TopLeftY, BotRightX, BotRightY ;
    STGetBarCodePos (hBarcode, i, &TopLeftX, &TopLeftY, &BotRightX, &BotRightY);
    //printf ("%s[%s]\n", bar_codes[i], bar_codes_type[i]);
    ret.push_back (bar_codes[i]);
    ret.push_back (bar_codes_type[i]);
  }
  
  STFreeBarCodeSession (hBarcode);
  
  return ret;
}
