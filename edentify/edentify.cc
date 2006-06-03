
/*
 * The ExactImage library's identify compatible command line frontend.
 * Copyright (C) 2006 René Rebe, Archivista
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
  Argument<bool> arg_help ("h", "help",
			   "display this help text and exit");
  arglist.Add (&arg_help);

  Argument<bool> arg_verbose ("v", "verbose",
			      "more verbose output");
  arglist.Add (&arg_verbose);

  Argument<std::string> arg_format ("f", "format",
				    "user defined format string",
				    0, 1);
  arglist.Add (&arg_format);
  
  // parse the specified argument list - and maybe output the Usage
  if (!arglist.Read (argc, argv))
    return 1;

  if (arg_help.Get() == true || arglist.Residuals().empty())
    {
      std::cerr << "Exact image identification (edentify)."
		<< std::endl << "Version " VERSION
                <<  " - Copyright (C) 2006 by René Rebe and Archivista" << std::endl
                << "Usage:" << std::endl;
      
      arglist.Usage (std::cerr);
      return 1;
    }
  
  // for all residual arguments (image files)
  int errors = 0;
  const std::vector<std::string>& list = arglist.Residuals();
  for (std::vector<std::string>::const_iterator it = list.begin();
       it != list.end(); ++it)
    {
      if (!ImageLoader::Read(*it, image)) {
	std::cout << "edentify: unable to open image '" << *it << "'." << std::endl;
	continue;
      }
      
      if (arg_format.Size() > 0)
	{
	  std::cout << "TODO: implement format handling: '"
		    << arg_format.Get() << "'" << std::endl;
	}
      else if (arg_verbose.Get())
	{
	  std::cout << "TODO: implement verbose output" << std::endl;
	}
      else {
	// TODO: attach the loader to the image, would be nice for JPEG, JP2
	// coefficent handling anyway, so we can querry the codec identity here
	// placeholder <CODEC> ...
	std::cout << *it << ": <CODEC> " << image.w << "x" << image.h;
	
	if (image.xres && image.yres)
	  std::cout << " @ " << image.xres << "x" << image.yres << "dpi ("
		    << 1000 * image.w / image.xres / 254 << "x"
		    << 1000 * image.h / image.yres / 254 << "mm)";
	
	int bits = image.bps * image.spp;
	std::cout << " " << bits << " bit" << (bits>1 ? "s" : "") << ", "
		  << image.spp << " channel" << (image.spp>1 ? "s" : "") << std::endl;
	
	std::cout << std::endl;
      }
    }
  return errors;
}
