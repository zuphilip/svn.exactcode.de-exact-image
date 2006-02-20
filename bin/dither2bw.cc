
/*
 * Copyright (C) 2006 René Rebe
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

#include "ArgumentList.hh"

#include "tiff.hh"
#include "jpeg.hh"
#include "Image.hh"

#include "riemersma.h"
#include "floyd-steinberg.h"

using namespace Utility;

int main (int argc, char* argv[])
{
  ArgumentList arglist;
  
  // setup the argument list
  Argument<bool> arg_help ("", "help",
			   "display this help text and exit");
  Argument<std::string> arg_input ("i", "input", "input file",
                                   1, 1);
  Argument<std::string> arg_output ("o", "output", "output file",
				    1, 1);
  Argument<int> arg_shades ("s", "shades",
                             "Number of shades to quantisize to.", 2, 0, 1);
  Argument<bool> arg_riem ("r", "riemersma",
                           "Riemersma dithering instead of Floyd-Steinberg");

  arglist.Add (&arg_help);
  arglist.Add (&arg_input);
  arglist.Add (&arg_output);
  arglist.Add (&arg_shades);
  arglist.Add (&arg_riem);

  // parse the specified argument list - and maybe output the Usage
  if (!arglist.Read (argc, argv) || arg_help.Get() == true)
    {
      std::cerr << "Color / Gray image to Bi-level dither"
                <<  " - Copyright 2006 by René Rebe" << std::endl
                << "Usage:" << std::endl;
      
      arglist.Usage (std::cerr);
      return 1;
    }
  
  Image image;
  if (!image.Read(arg_input.Get())) {
    std::cerr << "Error reading input file." << std::endl;
    return 1;
  }
  
  // convert to RGB to gray - TODO: more cases
  if (image.spp == 3 && image.bps == 8) {
    std::cerr << "RGB -> Gray convertion" << std::endl;
    
    unsigned char* output = image.data;
    unsigned char* input = image.data;
    
    for (int i = 0; i < image.w*image.h; i++)
      {
	// R G B order and associated weighting
	int c = (int)input [0] * 28;
	c += (int)input [1] * 59;
	c += (int)input [2] * 11;
	input += 3;
	
	*output++ = (unsigned char)(c / 100);
	
	image.spp = 1; // converted data right now
      }
  }
  else if (image.spp != 1 && image.bps != 8)
    {
      std::cerr << "Can't yet handle " << image.spp << " samples with "
		<< image.bps << " bits per sample." << std::endl;
      return 1;
    }

  // dither (in the gray data)
  if (arg_riem.Get() == false) {
    std::cout << "Using Floyd-Steinberg dithering ..." << std::endl;
    FloydSteinberg (image.data, image.w, image.h, arg_shades.Get());
  }
  else {
    std::cout << "Using Riemersma dithering ..." << std::endl;
    Riemersma (image.data, image.w, image.h, arg_shades.Get());
  }
  
  if (!image.Write(arg_output.Get())) {
    std::cerr << "Error writing output file." << std::endl;
    return 1;
  }
  
  return 0;
}
