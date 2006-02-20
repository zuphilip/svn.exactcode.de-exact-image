
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

#include <math.h>

#include <iostream>
#include <iomanip>

#include "ArgumentList.hh"

#include "Image.hh"
#include "Colorspace.hh"
#include "Matrix.hh"

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
  
  Argument<double> arg_sd ("sd", "standard-deviation",
			   "standard deviation for Gaussian distribution", 0.0, 0, 1);
  
  arglist.Add (&arg_help);
  arglist.Add (&arg_input);
  arglist.Add (&arg_output);

  // parse the specified argument list - and maybe output the Usage
  if (!arglist.Read (argc, argv) || arg_help.Get() == true)
    {
      std::cerr << "Yet simple edge detection"
                <<  " - Copyright 2005 by René Rebe" << std::endl
                << "Usage:" << std::endl;
      
      arglist.Usage (std::cerr);
      return 1;
    }
  
  Image image;
  if (!image.Read (arg_input.Get().c_str())) {
    std::cerr << "Error reading input file." << std::endl;
    return 1;
  }
  
  // convert to RGB to gray - TODO: more cases
  if (image.spp == 3 && image.bps == 8) {
    std::cerr << "RGB -> Gray convertion" << std::endl;
    
    colorspace_rgb_to_gray (image);
  }
  else if (image.spp != 1 && image.bps != 8)
    {
      std::cerr << "Can't yet handle " << image.spp << " samples with "
		<< image.bps << " bits per sample." << std::endl;
      return 1;
    }
  
  normalize (image);
  
  // Sobel edge detection 
  {
    int width = 3;
    matrix_type *matrix = new matrix_type[width * width];
  
    matrix [0 + (width * 0)] = -1.0;
    matrix [1 + (width * 0)] = 0.0;
    matrix [2 + (width * 0)] = 1.0;

    matrix [0 + (width * 1)] = -2.0;
    matrix [1 + (width * 1)] = 0.0;
    matrix [2 + (width * 1)] = 2.0;

    matrix [0 + (width * 2)] = -1.0;
    matrix [1 + (width * 2)] = 0.0;
    matrix [2 + (width * 2)] = -1.0;
    
    convolution_matrix (image, matrix, width, width, (matrix_type)3.0);
  }

  if (!image.Write (arg_output.Get())) {
    std::cerr << "Error writing output file." << std::endl;
    return 1;
  }
  
  return 0;
}
