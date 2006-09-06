
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
#include "Codecs.hh"

#include "Colorspace.hh"
#include "Matrix.hh"
#include "scale.hh"

#include "optimize2bw.hh"

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
  Argument<int> arg_low ("l", "low",
			 "low normalization value", 0, 0, 1);
  Argument<int> arg_high ("h", "high",
			  "high normalization value", 0, 0, 1);
  
  Argument<int> arg_threshold ("t", "threshold",
			       "threshold value", 0, 0, 1);

  Argument<int> arg_radius ("r", "radius",
			    "\"unsharp mask\" radius", 0, 0, 1);

  Argument<double> arg_scale ("s", "scale", "scale output by factor", 0.0, 0, 1);
  
  Argument<int> arg_dpi ("d", "dpi", "scale to specified DPI", 0, 0, 1);
  
  Argument<double> arg_sd ("sd", "standard-deviation",
			   "standard deviation for Gaussian distribution", 0.0, 0, 1);

  Argument<int> arg_lazy_thr ("z", "lazy", "do not unsharp mask values within threshold", 0, 0, 1);
  
  arglist.Add (&arg_help);
  arglist.Add (&arg_input);
  arglist.Add (&arg_output);
  arglist.Add (&arg_low);
  arglist.Add (&arg_high);
  arglist.Add (&arg_threshold);
  arglist.Add (&arg_radius);
  arglist.Add (&arg_scale);
  arglist.Add (&arg_dpi);
  arglist.Add (&arg_sd);
  arglist.Add (&arg_lazy_thr);

  // parse the specified argument list - and maybe output the Usage
  if (!arglist.Read (argc, argv) || arg_help.Get() == true)
    {
      std::cerr << "Color / Gray image to Bi-level optimizer"
                <<  " - Copyright 2005 by René Rebe" << std::endl
                << "Usage:" << std::endl;
      
      arglist.Usage (std::cerr);
      return 1;
    }
  
  Image image;
  if (!ImageCodec::Read (arg_input.Get(), image)) {
    std::cerr << "Error reading input file." << std::endl;
    return 1;
  }
  
  int low = 0;
  int high = 0;
  int sloppy_threshold = 0;
  int radius = 3;
  double sd = 2.1;
  
  if (arg_low.Get() != 0) {
    low = arg_low.Get();
    std::cerr << "Low value overwritten: " << low << std::endl;
  }
  
  if (arg_high.Get() != 0) {
    high = arg_high.Get();
    std::cerr << "High value overwritten: " << high << std::endl;
  }
  
  if (arg_radius.Get() != 0) {
    radius = arg_radius.Get();
    std::cerr << "Radius: " << radius << std::endl;
  }
  
  if (arg_sd.Get() != 0) {
    sd = arg_sd.Get();
    std::cerr << "SD overwritten: " << sd << std::endl;
  }
    
  optimize2bw (image, low, high, sloppy_threshold, radius, sd);
  
  // scale image using interpolation
  double scale = arg_scale.Get ();
  int dpi = arg_dpi.Get ();

  if (scale != 0.0 && dpi != 0) {
    std::cerr << "DPI and scale argument must not be specified at once!" << std::endl;
    return 1;
  }
  
  if (dpi != 0) {
    if (image.xres == 0)
      image.xres = image.yres;
    
    if (image.xres == 0) {
      std::cerr << "Image does not include DPI information!" << std::endl;
      return 1;
    }
    
    scale = (double)(dpi) / image.xres;
  }

  if (scale < 0.0) {
    std::cerr << "Scale must not be negativ!" << std::endl;
    return 1;
  }
  
  std::cerr << "Scale: " << scale << std::endl;
  
  if (scale > 0.0)
    {
      if (scale < 1.0)
	box_scale (image, scale, scale);
      else
	bilinear_scale (image, scale, scale);
    }
  
  
  // convert to 1-bit (threshold)
  int threshold = 170;
    
  if (arg_threshold.Get() != 0) {
    threshold = arg_threshold.Get();
    std::cerr << "Threshold: " << threshold << std::endl;
  }
  
  colorspace_gray8_to_gray1 (image, threshold);
  
  if (!ImageCodec::Write(arg_output.Get(), image)) {
    std::cerr << "Error writing output file." << std::endl;
    return 1;
  }
  
  return 0;
}
