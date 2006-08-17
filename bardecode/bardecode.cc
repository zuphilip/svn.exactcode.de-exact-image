
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

       #include <sys/types.h>
       #include <sys/stat.h>
       #include <fcntl.h>

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

  // we have a 1-8 bits per pixel GRAY image, now
  // build custom allocated bitmap to conform to the barcode library constraits
  // (4 byte row allignment ...)

  int stride = image.w / 8;
  std::cerr << "Stride: " << stride << std::endl;
  stride += stride % 4 > 0 ? 4 - stride % 4 : 0;
  std::cerr << "Stride: " << stride << std::endl;

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


  
  return 0;
}
