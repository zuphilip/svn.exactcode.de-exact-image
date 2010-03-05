/*
 * The ExactImage library's any to multi-page TIFF converted
 * Copyright (C) 2008 - 2010 René Rebe
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
#include <fstream>

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
                <<  "    - Copyright 2008 - 2010 by René Rebe, ExactCODE" << std::endl
                << "Usage:" << std::endl;
      
      arglist.Usage (std::cerr);
      return 1;
    }
  
  int errors = 0;
  int tiff_page = 1;
  std::fstream stream(arg_output.Get().c_str(), std::ios::in | std::ios::out | std::ios::trunc);
  ImageCodec* codec = ImageCodec::MultiWrite(&stream, "tiff", ImageCodec::getExtension(arg_output.Get()));
  if(!codec) {
    std::cerr << "Error writing file: " << arg_output.Get() << std::endl;
    return 1;
  }
  Image image;
  
  const std::vector<std::string>& filenames = arglist.Residuals();
  for (std::vector<std::string>::const_iterator file = filenames.begin();
       file != filenames.end ();
       ++file)
    {
      int n = 1;
      for (int i = 0; i < n; ++i)
      {
        int ret = ImageCodec::Read (*file, image, "", i);
	if (ret == 0) {
	  std::cerr << "Error reading " << *file << std::endl;
	  ++errors;
	  break;
        }
        else {
	  if (i == 0) n = ret;
	  // std::cerr << "adding [" << tiff_page << "] " << *file << std::endl;
	  codec->Write (image, 75, ""/*compression*/, tiff_page++);
        }
      }
    }
  
  delete(codec); codec = 0;

  return errors;
}
