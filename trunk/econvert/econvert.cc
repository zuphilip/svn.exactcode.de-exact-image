
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

bool convert_input (const Argument<std::string>& arg)
{
  if (image.data) {
    free (image.data);
    image.data = 0;
  }

  if (!ImageLoader::Read(arg.Get(), image)) {
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
  
  if (!ImageLoader::Write(arg.Get(), image)) {
    std::cerr << "Error writing output file." << std::endl;
    return false;
  }
  return true;
}

bool convert_split (const Argument<std::string>& arg)
{
  if (image.data == 0) {
    std::cerr << "No image available." << std::endl;
    return false;
  }
  
  if (!ImageLoader::Write(arg.Get(), image)) {
    std::cerr << "Error writing output file." << std::endl;
    return false;
  }

  Image split_image (image);
  
  // this is a bit ugly hacked for now
  split_image.h /= arg.count;
  if (split_image.h == 0) {
    std::cerr << "Resulting image size too small." << std::endl
	      << "The resulting slices must have at least a height of one pixel."
	      << std::endl;
    
    return false;
  }
  
  int err = 0;
  for (int i = 0; i < arg.Size(); ++i)
    {
      std::cout << "Writing file: " << arg.Get(i) << std::endl;
      split_image.data = image.data + i * split_image.Stride() * split_image.h;
      if (!ImageLoader::Write (arg.Get(i), split_image)) {
	err = 1;
	std::cerr << "Error writing output file." << std::endl;
      }
    }
  split_image.data = 0; // not deallocate in dtor
  
  return err == 0;
}

bool convert_colorspace (const Argument<std::string>& arg)
{
  const std::string& space = arg.Get();
  std::cerr << "convert_colorspace: " << space << std::endl;
  
  // up
  if (image.spp == 1 && image.bps == 1 &&
      (space == "GRAY" || space == "GRAY2" || space == "GRAY4" ||
       space == "RGB")) {
    if (space == "GRAY2")
      colorspace_gray1_to_gray2 (image);
    else if (space == "GRAY4")
      colorspace_gray1_to_gray4 (image);
    else
      colorspace_gray1_to_gray8 (image);

    if (space == "RGB")
      colorspace_gray8_to_rgb8 (image);
  }
  
  else if (image.spp == 1 && image.bps == 8 &&
	   space == "RGB") {
    colorspace_gray8_to_rgb8 (image);
  }
  
  // down
  else if (image.spp == 3 && image.bps == 8 &&
	   (space == "GRAY2" || space == "GRAY4"
	    || space == "GRAY" || space == "BW")) {
    colorspace_rgb8_to_gray8 (image);
    
    if (space == "GRAY4")
      colorspace_gray8_to_gray4 (image);
    else if (space == "GRAY2")
      colorspace_gray8_to_gray2 (image);
    if (space == "BW")
      colorspace_gray8_to_gray1 (image);
  }
  else if (image.spp == 1 && image.bps == 8 &&
	   space == "BW") {
    colorspace_gray8_to_gray1 (image);
  }
  else if (image.spp == 1 && image.bps == 8 &&
	   space == "GRAY4") {
    colorspace_gray8_to_gray4 (image);
  }
  else if (image.spp == 1 && image.bps == 8 &&
	   space == "GRAY2") {
    colorspace_gray8_to_gray2 (image);
  }
  else {
    std::cerr << "Requested colorspace conversion not yet implemented."
	      << std::endl;
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
  
  if (scale < 0.5)
    box_scale (image, scale, scale);
  if (scale == 2.0)
    directional_average_scale_2x (image);
  else if (scale > 2)
    bilinear_scale (image, scale, scale);
  else
    bicubic_scale (image, scale, scale);
  
  return true;
}

bool convert_near_scale (const Argument<double>& arg)
{
  double scale = arg.Get();
  nearest_scale (image, scale, scale);
  return true;
}

bool convert_bilinear_scale (const Argument<double>& arg)
{
  double scale = arg.Get();
  bilinear_scale (image, scale, scale);
  return true;
}

bool convert_bicubic_scale (const Argument<double>& arg)
{
  double scale = arg.Get();
  bicubic_scale (image, scale, scale);
  return true;
}

bool convert_box_scale (const Argument<double>& arg)
{
  double scale = arg.Get();
  box_scale (image, scale, scale);
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

bool convert_edge (const Argument<bool>& arg)
{
  matrix_type matrix[] = { -1.0, 0.0,  1.0,
                           -2.0, 0.0,  2.0,
                           -1.0, 0.0, -1.0};

  convolution_matrix (image, matrix, 3, 3, (matrix_type)3.0);

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
                                   0, 1, true, true);
  arg_input.Bind (convert_input);
  arglist.Add (&arg_input);
  
  Argument<std::string> arg_output ("o", "output", "output file",
				    0, 1, true, true);
  arg_output.Bind (convert_output);
  arglist.Add (&arg_output);
  
  Argument<std::string> arg_split ("", "split",
				   "filenames to save the images split into n parts",
				   0, 999, true, true);
  arg_split.Bind (convert_split);
  arglist.Add (&arg_split);
  
  Argument<std::string> arg_colorspace ("", "colorspace",
					"convert image colorspace",
					0, 1, true, true);
  arg_colorspace.Bind (convert_colorspace);
  arglist.Add (&arg_colorspace);

  Argument<bool> arg_normalize ("", "normalize",
				"transform the image to span the full color range",
				0, 1, true, true);
  arg_normalize.Bind (convert_normalize);
  arglist.Add (&arg_normalize);
  
  Argument<double> arg_scale ("", "scale",
			      "scale image data using a method suitable for the specified factor",
			      0.0, 0, 1, true, true);
  arg_scale.Bind (convert_scale);
  arglist.Add (&arg_scale);
  
  Argument<double> arg_near_scale ("", "near-scale",
				   "scale image data to nearest neighbour",
				   0.0, 0, 1, true, true);
  arg_near_scale.Bind (convert_near_scale);
  arglist.Add (&arg_near_scale);

  Argument<double> arg_bilinear_scale ("", "bilinear-scale",
				       "scale image data with bi-linear filter",
				       0.0, 0, 1, true, true);
  arg_bilinear_scale.Bind (convert_bilinear_scale);
  arglist.Add (&arg_bilinear_scale);

  Argument<double> arg_bicubic_scale ("", "bicubic-scale",
				      "scale image data with bi-cubic filter",
				      0.0, 0, 1, true, true);
  arg_bicubic_scale.Bind (convert_bicubic_scale);
  arglist.Add (&arg_bicubic_scale);

  Argument<double> arg_box_scale ("", "box-scale",
				   "(down)scale image data with box filter",
				  0.0, 0, 1, true, true);
  arg_box_scale.Bind (convert_box_scale);
  arglist.Add (&arg_box_scale);

  Argument<int> arg_rotate ("", "rotate",
			    "rotation angle",
			    0, 1, true, true);
  arg_rotate.Bind (convert_rotate);
  arglist.Add (&arg_rotate);

  Argument<bool> arg_flip ("", "flip",
			   "flip the image vertically",
			   0, 1, true, true);
  arg_flip.Bind (convert_flip);
  arglist.Add (&arg_flip);

  Argument<bool> arg_flop ("", "flop",
			   "flip the image horizontally",
			   0, 1, true, true);
  arg_flop.Bind (convert_flop);
  arglist.Add (&arg_flop);

  Argument<int> arg_floyd ("", "floyd-steinberg",
			   "Floyd Steinberg dithering using n shades",
			   0, 1, true, true);
  arg_floyd.Bind (convert_dither_floyd_steinberg);
  arglist.Add (&arg_floyd);
  
  Argument<int> arg_riemersma ("", "riemersma",
			       "Riemersma dithering using n shades",
			       0, 1, true, true);
  arg_riemersma.Bind (convert_dither_riemersma);
  arglist.Add (&arg_riemersma);


  Argument<bool> arg_edge ("", "edge",
                           "Edge detect",
			   0, 1, true, true);
  arg_edge.Bind (convert_edge);
  arglist.Add (&arg_edge);
  
  
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
