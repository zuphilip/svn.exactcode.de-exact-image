
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

#include "tiff.h"
#include "jpeg.h"

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
  
  arglist.Add (&arg_help);
  arglist.Add (&arg_input);
  arglist.Add (&arg_output);
  arglist.Add (&arg_low);
  arglist.Add (&arg_high);
  arglist.Add (&arg_threshold);
  arglist.Add (&arg_radius);
  arglist.Add (&arg_scale);
  arglist.Add (&arg_dpi);

  // parse the specified argument list - and maybe output the Usage
  if (!arglist.Read (argc, argv) || arg_help.Get() == true)
    {
      std::cerr << "Color / Gray image to Bi-level optimizer"
                <<  " - Copyright 2005 by René Rebe" << std::endl
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
  
  std::cerr << "xres: " << xres << ", yres: " << yres << std::endl;
  
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
  
  {
    int histogram[256] = { 0 };
    for (int i = 0; i < h * w; i++)
      histogram[data[i]]++;

    int lowest = 255, highest = 0;
    for (int i = 0; i <= 255; i++)
      {
	// std::cout << i << ": "<< histogram[i] << std::endl;
	if (histogram[i] > 2) // magic denoise constant
	  {
	    if (i < lowest)
	      lowest = i;
	    if (i > highest)
	      highest = i;
	  }
      }
    std::cerr << "lowest: " << lowest << " - highest: "
	      << highest << std::endl;
    
    if (arg_low.Get() != 0) {
      lowest = arg_low.Get();
      std::cerr << "Low value overwritten: " << lowest << std::endl;
    }
    
    if (arg_high.Get() != 0) {
      highest = arg_high.Get();
      std::cerr << "High value overwritten: " << highest << std::endl;
    }
    
    // TODO use options
    signed int a = (255 * 256) / (highest - lowest);
    signed int b = -a * lowest;

    std::cerr << "a: " << (float) a / 256
	      << " b: " << (float) b / 256 << std::endl;
    for (int i = 0; i < h * w; i++)
      data[i] = ((int) data[i] * a + b) / 256;
  }

  // Convolution Matrix (unsharp mask a-like)
  unsigned char *data2 = (unsigned char *) malloc (w * h);
  {
    // any matrix and devisior

    typedef float matrix_type;

    // compute kernel (convolution matrix to move over the iamge)
    
    int radius = 3;
    if (arg_radius.Get() != 0) {
      radius = arg_radius.Get();
      std::cerr << "Radius: " << radius << std::endl;
    }

    int width = radius * 2 + 1;
    matrix_type divisor = 3;
    float sd = 1;
    
    
    matrix_type *matrix = new matrix_type[width * width];
    
    std::cout << std::fixed << std::setprecision(3);
    for (int y = -radius; y <= radius; y++) {
      for (int x = -radius; x <= radius; x++) {
	double v = - exp (-((float)x*x + (float)y*y) / ((float)2 * sd * sd));
	if (x == 0 && y == 0)
	  v *= -8;
	std::cout << v << " ";
	matrix[x + radius + (y+radius)*width] = v;
      }
      std::cout << std::endl;
    }
    
    for (int y = 0; y < h; y++)
      {
	for (int x = 0; x < w; x++)
	  {
	    // for now copy border pixels
	    if (y < radius || y > h - radius ||
		x < radius || x > w - radius)
	      data2[x + y * w] = data[x + y * w];
	    else
	      {
		matrix_type sum = 0;
		for (int y2 = 0; y2 < width; y2++)
		  {
		    matrix_type* matrix_row = &matrix [y2 * width];
		    unsigned char* data_row = &data[ ((y - radius + y2) * w) - radius + x];
		    
		    for (int x2 = 0; x2 < width; x2++)
		      {
			matrix_type v = data_row[x2];
			sum += v * matrix_row [x2];
			/*if (y == h/2 && x == w/2)
			  std::cout << sum << std::endl; */
		      }
		}
		
		sum /= divisor;
		if (y == h/2 && x == w/2)
		  std::cout << sum << std::endl;
		unsigned char z = (unsigned char)
		  (sum > 255 ? 255 : sum < 0 ? 0 : sum);
		data2[x + y * w] = z;
	      }
	  }
      }
  }
  data = data2;
  
// #define DEBUG
  
  // scale image using interpolation
  
  double scale = arg_scale.Get ();
  int dpi = arg_dpi.Get ();

  if (scale != 0.0 && dpi != 0) {
    std::cerr << "DPI and scale argument must not be specified at once!" << std::endl;
    return 1;
  }
  
  if (dpi != 0) {
    if (xres == 0)
      xres = yres;
    
    if (xres == 0) {
      std::cerr << "Image does not include DPI information!" << std::endl;
      return 1;
    }
    
    scale = (double)(dpi) / xres;
  }
  
  if (scale != 0.0 && scale < 1.0) {
    std::cerr << "Downscaling not yet implemented!" << std::endl;
    return 1;
  }
  
  std::cerr << "Scale: " << scale << std::endl;
  
  if (scale > 1.0) {

    int wn = (int) (scale * (double) w);
    int hn = (int) (scale * (double) h);

    xres = (int) (scale * xres);
    yres = (int) (scale * yres);

    scale = 256.0 / scale;

    std::cerr << "new dimensions: " << wn << " x " << hn 
	      << " (xres: " << xres << ", yres: " << yres << ")" << std::endl;

    unsigned char* ndata = (unsigned char*) malloc (wn * hn);

    unsigned int offset = 0;
    for (int y=0; y < hn; y++)
      for (int x=0; x < wn; x++) {

	int bx = (int) (((double) x) * scale);
	int by = (int) (((double) y) * scale);
	
	int sx = std::min(bx / 256, w - 1);
	int sy = std::min(by / 256, h - 1);
	int sxx = std::min(sx + 1, w - 1);
	int syy = std::min(sy + 1, h - 1);

	int fxx = bx % 256;
	int fyy = by % 256;
	int fx = 256 - fxx;
	int fy = 256 - fyy;
 
	unsigned int value
	  = fx  * fy  * ( (unsigned int) data [sx  + w * sy ])
	  + fxx * fy  * ( (unsigned int) data [sxx + w * sy ])
	  + fx  * fyy * ( (unsigned int) data [sx  + w * syy])
	  + fxx * fyy * ( (unsigned int) data [sxx + w * syy]);

	value /= 256 * 256;
	value = std::min (value, (unsigned int) 255);

	ndata[offset++] = (unsigned char) value;
      }
    
    data = ndata;
    w = wn;
    h = hn;
  }
  
#ifdef DEBUG
  std::cout << "w: " << w << ", h: " << h << std::endl;
  FILE* f = fopen ("optimized.raw", "w+");
  fwrite (data, w * h, 1, f);
  fclose(f);
#endif

  // convert to 1-bit (threshold)
  
  unsigned char *output = data;
  unsigned char *input = data;
  
  int threshold = 127;
    
  if (arg_threshold.Get() != 0) {
    threshold = arg_threshold.Get();
    std::cerr << "Threshold: " << threshold << std::endl;
  }
    
  for (int row = 0; row < h; row++)
    {
      unsigned char z = 0;
      int x = 0;
      for (; x < w; x++)
	{
	  z <<= 1;
	  if (*input++ > threshold)
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
  
  free (data2);

  return 0;
}
