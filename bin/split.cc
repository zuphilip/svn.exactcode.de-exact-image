
/*
 * Copyright (C) 2006 René Rebe
 *           (C) 2006 Archivista GmbH, CH-8042 Zuerich
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
#include "Image.hh"

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
				    2, 99);
  
  arglist.Add (&arg_help);
  arglist.Add (&arg_input);
  arglist.Add (&arg_output);

  // parse the specified argument list - and maybe output the Usage
  if (!arglist.Read (argc, argv) || arg_help.Get() == true)
    {
      std::cerr << "Image splitting"
                <<  " - Copyright 2006 by René Rebe" << std::endl
                << "Usage:" << std::endl;
      
      arglist.Usage (std::cerr);
      return 1;
    }
  
  Image image;
  image.data = read_TIFF_file (arg_input.Get().c_str(),
			       &image.w, &image.h,
			       &image.bps, &image.spp,
			       &image.xres, &image.yres);
  if (!image.data) {
    std::cerr << "Error reading JPEG." << std::endl;
    return 1;
  }
  
  // this is a bit ugly hacked for now
  image.h /= arg_output.count;
  int stride = (image.w * image.spp * image.bps + 7) / 8;
  
  if (image.h == 0) {
    std::cerr << "Resulting image size too small." << std::endl
	      << "The resulting slices must have at least a height of one pixel."
	      << std::endl;
    return 1;
  }
  
  // TODO: allow obtaining read argument count, missing in the utility
  // right now it seems
  for (int i = 0; i < arg_output.count; ++i)
    {
      std::cout << "Writing file: " << arg_output.Get(i) << std::endl;
      write_TIFF_file (arg_output.Get(i).c_str(),
		       image.data + i * stride * image.h,
		       image.w, image.h,
		       image.bps, image.spp, image.xres, image.yres);
    }
  
  return 0;
}
