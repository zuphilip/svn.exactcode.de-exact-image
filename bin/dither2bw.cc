
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
  
  arglist.Add (&arg_help);
  arglist.Add (&arg_input);
  arglist.Add (&arg_output);

  // parse the specified argument list - and maybe output the Usage
  if (!arglist.Read (argc, argv) || arg_help.Get() == true)
    {
      std::cerr << "Color / Gray image to Bi-level dither"
                <<  " - Copyright 2006 by René Rebe" << std::endl
                << "Usage:" << std::endl;
      
      arglist.Usage (std::cerr);
      return 1;
    }
  
  int w, h, bps, spp, xres, yres;
  unsigned char* data = read_JPEG_file (arg_input.Get().c_str(),
					&w, &h, &bps, &spp, &xres, &yres);
  if (!data)
  {
    std::cerr << "Error reading JPEG." << std::endl;
    return 1;
  }
  
  // convert to RGB to gray - TODO: more cases
  if (spp == 3 && bps == 8) {
    std::cerr << "RGB -> Gray convertion" << std::endl;
    
    unsigned char* output = data;
    unsigned char* input = data;
    
    for (int i = 0; i < w*h; i++)
      {
	// R G B order and associated weighting
	int c = (int)input [0] * 28;
	c += (int)input [1] * 59;
	c += (int)input [2] * 11;
	input += 3;
	
	*output++ = (unsigned char)(c / 100);
	
	spp = 1; // converted data right now
      }
  }
  else if (spp != 1 && bps != 8)
    {
      std::cerr << "Can't yet handle " << spp << " samples with "
		<< bps << " bits per sample." << std::endl;
      return 1;
    }

  // dither (in the gray data)
  //FloydSteinberg(data,w,h,2);
  Riemersma(data,w,h);
  
  // convert to 1-bit (threshold)
  
  unsigned char *output = data;
  unsigned char *input = data;

  for (int row = 0; row < h; row++)
    {
      unsigned char z = 0;
      int x = 0;
      for (; x < w; x++)
	{
	  z <<= 1;
	  if (*input++ > 128)
	    z |= 0x01;

	  if (x % 8 == 7)
	    {
	      *output++ = z;
	      z = 0;
	    }
	}
      // remainder - TODO: test for correctness ...
      int remainder = 8 - x % 8;
      if (remainder != 8)
	{
	  z <<= remainder;
	  *output++ = z;
	}
    }

  // new image data - and 8 pixel align due to packing nature
  w = ((w + 7) / 8) * 8;
  bps = 1;

  write_TIFF_file (arg_output.Get().c_str(), data, w, h, bps, spp,
		   xres, yres);
  
  return 0;
}
