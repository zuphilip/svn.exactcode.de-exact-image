
/*
 * Copyright (C) 2005 René Rebe
 *           (C) 2005 Archivista GmbH, CH-8042 Zuerich
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

#include <iostream>

#include "tiff.hh"
#include "jpeg.hh"
#include "Image.hh"

#include "ArgumentList.hh"

using namespace Utility;

/* TODO: for more accurance one could introduce a hot-spot area that
   has a higher weight than the other (outer) region to more reliably
   detect crossed but otherwise empty pages */

/* TODO: include color / gray -> bi-level optimization and other file
   loading */

int main (int argc, char* argv[])
{
  ArgumentList arglist;
  
  // setup the argument list
  Argument<bool> arg_help ("", "help",
			   "display this help text and exit");
  Argument<std::string> arg_input ("i", "input", "input file",
                                   1, 1);
  Argument<int> arg_margin ("m", "margin",
			    "border margin to skip", 16, 0, 1);
  Argument<float> arg_percent ("p", "percentage",
			       "coverate for non-empty page", 0.05, 0, 1);
  
  arglist.Add (&arg_help);
  arglist.Add (&arg_input);
  arglist.Add (&arg_margin);
  arglist.Add (&arg_percent);
  
  // parse the specified argument list - and maybe output the Usage
  if (!arglist.Read (argc, argv) || arg_help.Get() == true)
    {
      std::cerr << "Empty page detector"
                <<  " - Copyright 2005 by René Rebe" << std::endl
                << "Usage:" << std::endl;
      
      arglist.Usage (std::cerr);
      return 1;
    }
  
  const int margin = arg_margin.Get();
  if (margin % 8 != 0) {
    std::cerr << "For speed reasons, the margin has to be a multiple of 8."
	      << std::endl;
    return 1;
  }
  
  Image image;
  if (!image.Read(arg_input.Get())) {
    std::cerr << "Error reading input file." << std::endl;
    return 1;
  }
  
  // if not 1-bit optimize
  if (image.spp != 1 || image.bps != 1)
    {
      std::cerr << "Non-bilevel image - needs optimzation, to be done."
		<< std::endl;
      
      return 2;
    }
  
  // count bits and decide based on that
  
  // create a bit table for fast lookup
  int bits_set[256] = { 0 };
  for (int i = 0; i < 256; i++) {
    int bits = 0;
    for (int j = i; j != 0; j >>= 1) {
      bits += (j & 0x01);
    }
    bits_set[i] = bits;
  }
  
  int stride = (image.w * image.bps * image.spp + 7) / 8;
  
  // count pixels
  int pixels = 0;
  for (int row = margin; row < image.h-margin; row++) {
    for (int x = margin/8; x < stride - margin/8; x++) {
      int b = bits_set [ image.data[stride*row + x] ];
      // it is a bits_set table - and we want the zeros ...
      pixels += 8-b;
    }
  }

  float percentage = (float)pixels/(image.w*image.h) * 100;
  std::cout << "The image has " << pixels << " dark pixels from a total of "
	    << image.w*image.h << " (" << percentage << "%)." << std::endl;
  
  if (percentage > arg_percent.Get()) {
    std::cout << "non-empty" << std::endl;
    return 1;
  }

  std::cout << "empty" << std::endl;
  return 0;
}
