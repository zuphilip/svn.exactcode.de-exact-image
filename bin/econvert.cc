
/*
 * The ExactImage libraries convert compativle command line frontend.
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

#include "ArgumentList.hh"

#include "tiff.hh"
#include "jpeg.hh"
#include "Image.hh"

#include "Colorspace.hh"

#include "scale.hh"
#include "rotate.hh"
#include "riemersma.h"
#include "floyd-steinberg.h"

#include <functional>

using namespace Utility;

Image image; // the global Image we work on

bool convert_input (const Argument<std::string>& arg)
{
  if (image.data) {
    free (image.data);
    image.data = 0;
  }

  if (!image.Read(arg.Get())) {
    std::cerr << "Error reading input file." << std::endl;
    return false;
  }
  return true;
}

bool convert_output (const Argument<std::string>& arg)
{
  if (image.data == 0) {
    std::cerr << "No image available." << std::endl;
    return false;
  }
  
  if (!image.Write(arg.Get())) {
    std::cerr << "Error writing output file." << std::endl;
    return false;
  }
  return true;
}

bool convert_colorspace (const Argument<std::string>& arg)
{
  const std::string& space = arg.Get();
  std::cerr << "convert_colorspace: " << space << std::endl;
  
  // TODO: implement in all variants
  if (image.spp == 3 && (space == "GRAY" || space == "BW")) {
    colorspace_rgb_to_gray (image);
    if (space == "BW")
      colorspace_gray_to_bilevel (image);
  }
  else if (image.spp == 1 && image.bps > 1 && space == "BW") {
    colorspace_gray_to_bilevel (image);
  }
  else {
    std::cerr << "Requested colorspace conversion not yet implemented." << std::endl;
    return false;
  }
  
  return true;
}

bool convert_normalize (const Argument<bool>& arg)
{
  normalize (image);
  return true;
}

bool convert_scale (const Argument<double>& arg)
{
  double scale = arg.Get();
   
  linear_scale (image, scale);
  return true;
}

bool convert_rotate (const Argument<int>& arg)
{
  int angle = arg.Get() % 360;
   
  rotate (image, angle);
  return true;
}

bool convert_flip (const Argument<bool>& arg)
{
  flipY (image);
  return true;
}

bool convert_flop (const Argument<bool>& arg)
{
  flipX (image);
  return true;
}

bool convert_dither_floyd_steinberg (const Argument<int>& arg)
{
  if (image.spp != 1 || image.bps != 8) {
    std::cerr << "Can only dither GRAY data right now." << std::endl;
    return false;
  }
  FloydSteinberg (image.data, image.w, image.h, arg.Get());
  return true;
}

bool convert_dither_riemersma (const Argument<int>& arg)
{
  if (image.spp != 1 || image.bps != 8) {
    std::cerr << "Can only dither GRAY data right now." << std::endl;
    return false;
  }
  Riemersma (image.data, image.w, image.h, arg.Get());
  return true;
}

int main (int argc, char* argv[])
{
  ArgumentList arglist;
  
  // setup the argument list
  Argument<bool> arg_help ("", "help",
			   "display this help text and exit");
  arglist.Add (&arg_help);
  
  Argument<std::string> arg_input ("i", "input", "input file",
                                   1, 1);
  arg_input.Bind (convert_input);
  arglist.Add (&arg_input);
  
  Argument<std::string> arg_output ("o", "output", "output file",
				    1, 1);
  arg_output.Bind (convert_output);
  arglist.Add (&arg_output);

  Argument<std::string> arg_colorspace ("", "colorspace",
					"convert image colorspace", 0, 1);
  arg_colorspace.Bind (convert_colorspace);
  arglist.Add (&arg_colorspace);

  Argument<bool> arg_normalize ("", "normalize",
				"transform the image to span the full color range");
  arg_normalize.Bind (convert_normalize);
  arglist.Add (&arg_normalize);

  Argument<double> arg_scale ("", "scale",
			      "scale image data", 0.0, 0, 1);
  arg_scale.Bind (convert_scale);
  arglist.Add (&arg_scale);

  Argument<int> arg_rotate ("", "rotate",
			    "rotation angle", 0, 0, 1);
  arg_rotate.Bind (convert_rotate);
  arglist.Add (&arg_rotate);

  Argument<bool> arg_flip ("", "flip",
			   "flip the image vertically");
  arg_flip.Bind (convert_flip);
  arglist.Add (&arg_flip);

  Argument<bool> arg_flop ("", "flop",
			   "flip the image horizontally");
  arg_flop.Bind (convert_flop);
  arglist.Add (&arg_flop);

  Argument<int> arg_floyd ("", "floyd-steinberg",
			   "Floyd Steinberg dithering using n shades", 0, 0, 1);
  arg_floyd.Bind (convert_dither_floyd_steinberg);
  arglist.Add (&arg_floyd);
  
  Argument<int> arg_riemersma ("", "riemersma",
			   "Riemersma dithering using n shades", 0, 0, 1);
  arg_riemersma.Bind (convert_dither_riemersma);
  arglist.Add (&arg_riemersma);
  
  
  // parse the specified argument list - and maybe output the Usage
  if (!arglist.Read (argc, argv) || arg_help.Get() == true)
    {
      std::cerr << "Exact image converter including a variety of fast algorithms."
		<< std::endl
                <<  " - Copyright 2005, 2006 by René Rebe and Archivista" << std::endl
                << "Usage:" << std::endl;
      
      arglist.Usage (std::cerr);
      return 1;
    }
  
  
  // all is done inside the argument callback functions
  return 0;
}
