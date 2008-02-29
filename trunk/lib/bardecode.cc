/*
 * Bardecode frontend with ExactImage image i/o backend."
 * Copyright (C) 2006 - 2008 René Rebe for Archivista
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

const bool debug = false;

std::vector<std::string> decodeBarcodes (Image& im, const std::string& codes,
					 unsigned int min_length,
                                         unsigned int max_length, int multiple)
{
  uint16_t i;
  
  // a copy we can mangle
  Image* image = new Image;
  *image = im;
  
  if (image->xres == 0)
    image->xres = 300; // passed down the lib ...
  
  // the barcode library does not support such a high bit-depth
  if (image->bps == 16)
    colorspace_16_to_8 (*image);
  
  // the library interface only handles one channel data
  if (image->spp == 3) // color crashes the library more often than not
    colorspace_rgb8_to_gray8 (*image);
  
  // the library does not appear to like 2bps ?
  if (image->bps == 2)
    colorspace_grayX_to_gray8 (*image);
  
  // now we have a 1, 4 or 8 bits per pixel GRAY image
  
  //ImageCodec::Write ("dump.tif", *image, 90, "");
  
  // The bardecode library is documented to require a 4 byte row
  // allignment. To conform this a custom allocated bitmap would
  // be required which would either require a complete Image class
  // rewrite or a extremely costly allocation and copy at this
  // location. Depending on the moon this is required or not.
  
  uint8_t* malloced_data = image->getRawData();
  {
    // required alignments
    const int base_align = 4;
    const int stride_align = 4;
    
    int stride = image->stride ();
    int new_stride = (stride + stride_align - 1) / stride_align * stride_align;
    
    // realloc the data to the maximal working set of memory we
    // might have to work with in the worst-case
    image->setRawDataWithoutDelete ((uint8_t*)
      realloc (image->getRawData(), new_stride * image->h + base_align));
    malloced_data = image->getRawData();
    uint8_t* new_data = (uint8_t*) (((long)image->getRawData() + base_align - 1) & ~(base_align-1));
    
    if (debug) {
      std::cerr << "  stride: " << stride << " aligned: " << new_stride << std::endl;
      std::cerr << "  @: " << (void*) image->getRawData()
		<< " aligned: " << (void*) new_data << std::endl;
    }
    
    if (stride != new_stride || image->getRawData() != new_data)
      {
	if (debug)
	  std::cerr << "  moving data ..." << std::endl;
	
	for (int y = image->h-1; y >= 0; --y) {
	  memmove (new_data + y*new_stride, image->getRawData() + y*stride, stride);
	  memset (new_data + y*new_stride + stride, 0xff, new_stride-stride);
       }
	
	// store new stride == width (@ 8bit gray)
	image->w = new_stride * 8 / image->bps;
	image->setRawDataWithoutDelete (new_data);
      }
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
          else if (code == "any") {
	      STSetParameter (hBarcode, ST_READ_CODE39, &i);
	      STSetParameter (hBarcode, ST_READ_CODE128, &i);
	      STSetParameter (hBarcode, ST_READ_CODE25, &i);
	      STSetParameter (hBarcode, ST_READ_EAN13, &i);
	      STSetParameter (hBarcode, ST_READ_EAN8, &i);
	      STSetParameter (hBarcode, ST_READ_UPCA, &i);
	      STSetParameter (hBarcode, ST_READ_UPCE, &i);
	  }
	  else
	    std::cerr << "Unrecognized barcode type: " << code << std::endl;
	}
    }
  while (it2 != std::string::npos);
  
  // only set if non-zero, otherwise CODE39 with chars does
  // not appear to work quite right
  if (min_length) {
    i = min_length;
    STSetParameter (hBarcode, ST_MIN_LEN, &i);
  }
  
  if (max_length) {
    i = max_length;
    STSetParameter (hBarcode, ST_MAX_LEN, &i);
  }
  
  if (false) // the library has defaults?
    {
      i = 15; // all directions
      STSetParameter(hBarcode, ST_ORIENTATION_MASK, &i);
      
      i = 20;
      STSetParameter(hBarcode, ST_NOISEREDUCTION, &i);

      i = 1;
      STSetParameter(hBarcode, ST_DESPECKLE, &i);
      
      i = 166;
      STSetParameter(hBarcode, ST_CONTRAST, &i);
    }

  i = multiple;
  STSetParameter(hBarcode, ST_MULTIPLE_READ, &i);

  
  BITMAP bbitmap;
  bbitmap.bmType = 1; // bitmap type version, fixed v1
  bbitmap.bmWidth = image->w;
  bbitmap.bmHeight = image->h;
  bbitmap.bmWidthBytes = image->stride();
  bbitmap.bmPlanes = 1; // the library is documented to only take 1
  bbitmap.bmBitsPixel = image->bps * image->spp; // 1, 4 and 8 appeared to work
  bbitmap.bmBits = image->getRawData(); // our class' bitmap data
  
  if (debug)
    std::cerr << "  @: " << (void*) image->getRawData()
	      << ", w: " << image->w << ", h: " << image->h
	      << ", spp: " << image->spp << ", bps: " << image->bps
	      << ", stride: " << image->stride()
	      << ", res: " << image->xres << std::endl;
  
  char** bar_codes;
  char** bar_codes_type;
  
  // 0 == photometric min is black, but this appears to be inverted?
  int photometric = 1;
  int bar_count = STReadBarCodeFromBitmap (hBarcode, &bbitmap, image->xres,
					   &bar_codes, &bar_codes_type,
					   photometric);
  std::vector<std::string> ret;
  
  for (i = 0; i < bar_count; ++i) {
    uint32 TopLeftX, TopLeftY, BotRightX, BotRightY ;
    STGetBarCodePos (hBarcode, i, &TopLeftX, &TopLeftY,
		     &BotRightX, &BotRightY);
    //printf ("%s[%s]\n", bar_codes[i], bar_codes_type[i]);
    ret.push_back (bar_codes[i]);
    ret.push_back (bar_codes_type[i]);
  }
  
  STFreeBarCodeSession (hBarcode);
  
  // as this one needs to be free'd
  image->setRawDataWithoutDelete (malloced_data);
  delete (image); image = 0;
  
  return ret;
}
