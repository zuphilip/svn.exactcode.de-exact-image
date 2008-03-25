/*
 * The ExactImage library's any to multi-page TIFF converted
 * Copyright (C) 2008 René Rebe
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
 */

#include <math.h>

#include <iostream>
#include <iomanip>
#include <map>
#include <cctype>

#include "tiffio.h"
#include "tiff.hh"

#include "ArgumentList.hh"
#include "Codecs.hh"

using namespace Utility;

int main (int argc, char* argv[])
{
  ArgumentList arglist (true); // enable residual gathering
  
  // setup the argument list
  Argument<bool> arg_help ("", "help",
			   "display this help text and exit");
  arglist.Add (&arg_help);
  
  Argument<std::string> arg_output ("o", "output",
				    "output file",
				    1, 1, true, true);
  arglist.Add (&arg_output);
  
  // parse the specified argument list - and maybe output the Usage
  if (!arglist.Read (argc, argv) || arg_help.Get() == true ||
      arglist.Residuals().empty())
    {
      std::cerr << "any to multi-page TIFF convert" << std::endl
                <<  "    - Copyright 2008 by René Rebe, ExactCODE" << std::endl
                << "Usage:" << std::endl;
      
      arglist.Usage (std::cerr);
      return 1;
    }
  
  int errors = 0;
  int tiff_page = 1;
  TIFF* tiff_context = TIFFOpen (arg_output.Get().c_str(), "w");
  
  Image image;
  
  const std::vector<std::string>& filenames = arglist.Residuals();
  for (std::vector<std::string>::const_iterator file = filenames.begin();
       file != filenames.end ();
       ++file)
    {
      if (!ImageCodec::Read (*file, image)) {
	std::cerr << "Error reading " << *file << std::endl;
	++errors;
	continue;
      }
      else {
	// std::cerr << "adding [" << tiff_page << "] " << *file << std::endl;
	TIFCodec::writeImageImpl (tiff_context, image, ""/*compression*/, tiff_page++);
      }
    }
  
  TIFFClose (tiff_context);
  tiff_context = NULL;

  return errors;
}
