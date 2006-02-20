
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

#include "tiff.hh"
#include "jpeg.hh"
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
  image.data = read_JPEG_file (arg_input.Get().c_str(),
			       &image.w, &image.h, &image.bps, &image.spp,
			       &image.xres, &image.yres);
  if (!image.data) {
    std::cerr << "Error reading JPEG." << std::endl;
    return 1;
  }
  
  std::cerr << "xres: " << image.xres << ", yres: " << image.yres << std::endl;
  
  // convert to RGB to gray - TODO: more cases
  if (image.spp == 3 && image.bps == 8) {
    std::cerr << "RGB -> Gray convertion" << std::endl;
    
    unsigned char* output = image.data;
    for (unsigned char* it = image.data; it < image.data + image.w*image.h*image.spp;)
      {
	// R G B order and associated weighting
	int c = (int)*it++ * 28;
	c += (int)*it++ * 59;
	c += (int)*it++ * 11;
	
	*output++ = (unsigned char)(c / 100);
      }
    image.spp = 1; // converted data right now
  }
  else if (image.spp != 1 && image.bps != 8)
    {
      std::cerr << "Can't yet handle " << image.spp << " samples with "
		<< image.bps << " bits per sample." << std::endl;
      return 1;
    }
  
  {
    int histogram[256] = { 0 };
    for (unsigned char* it = image.data; it < image.data + image.w*image.h;)
      histogram[*it++]++;

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

   for (unsigned char* it = image.data; it < image.data + image.w*image.h; ++it)
     *it = ((int) *it * a + b) / 256;
  }
  
  // Convolution Matrix (unsharp mask a-like)
  unsigned char *data2 = (unsigned char *) malloc (image.w * image.h);
  {
    // any matrix and devisior
    // on my AMD Turion speed is: double > int > float
    // and in 32bit mode: double > float > int ?
    typedef double matrix_type;

    // compute kernel (convolution matrix to move over the iamge)
    
    int radius = 3;
    if (arg_radius.Get() != 0) {
      radius = arg_radius.Get();
      std::cerr << "Radius: " << radius << std::endl;
    }
    
    const int width = radius * 2 + 1;
    matrix_type divisor = 0;
    float sd = 2.1;
    
    if (arg_sd.Get() != 0) {
      sd = arg_sd.Get();
      std::cerr << "SD overwritten: " << sd << std::endl;
    }
    
    matrix_type *matrix = new matrix_type[width * width];
    
    std::cout << std::fixed << std::setprecision(3);
    for (int y = -radius; y <= radius; y++) {
      for (int x = -radius; x <= radius; x++) {
	matrix_type v = (matrix_type) (exp (-((float)x*x + (float)y*y) / (2. * sd * sd)) * 5);
	divisor += v;
	
	matrix[x + radius + (y+radius)*width] = v;
      }
    }
    
    // sub from image *2 and print
    for (int y = -radius; y <= radius; y++) {
      for (int x = -radius; x <= radius; x++) {
	matrix_type* m = &matrix[x + radius + (y+radius)*width];
	
	*m *= -1;
	if (x == 0 && y == 0)
	  *m += 2*divisor;
	
	std::cout << *m << " ";
      }
      std::cout << std::endl;
    }
    std::cout << "Divisor: " << divisor << std::endl;

    const int sloppy_thr = arg_lazy_thr.Get();
    std::cout << "Lazy threshold: " << sloppy_thr << std::endl;
#if 0
    const int tiles = 16;
    for (int my = 0; my < h; my += tiles)
      for (int mx = 0; mx < w; mx += tiles)
	for (int y = my; y < my + tiles && y < h; ++y)
	  for (int x = mx; x < mx + tiles && x < w; ++x)
#else
    for (int y = 0; y < image.h; ++y)
      for (int x = 0; x < image.w; ++x)
#endif
	  {
	    unsigned char * const dst_ptr = &data2[x + y * image.w];
	    unsigned char * const src_ptr = &image.data[x + y * image.w];

	    const unsigned char val = *src_ptr;

	    // for now copy border pixels
	    if (y < radius || y > image.h - radius ||
		x < radius || x > image.w - radius)
	      *dst_ptr = val;
	    else if (!(val > sloppy_thr && val < 255-sloppy_thr))
	      *dst_ptr = val;
	    else {
	        matrix_type sum = 0;
		unsigned char* data_row =
		  &image.data[ (y - radius) * image.w - radius + x];
	        matrix_type* matrix_row = matrix;
	  	// in former times this was more readable and got overoptimized
		// for speed ,-)
		for (int y2 = 0; y2 < width; ++y2, data_row += image.w - width)
		  for (int x2 = 0; x2 < width; ++x2)
		    sum += *data_row++ * *matrix_row++;
		
	    sum /= divisor;
	    unsigned char z = (unsigned char)
	      (sum > 255 ? 255 : sum < 0 ? 0 : sum);
	    *dst_ptr = z;
	    }
	  }
  }
  
  free (image.data);
  image.data = data2;
  
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
  
  if (scale > 0.0) {

    int wn = (int) (scale * (double) image.w);
    int hn = (int) (scale * (double) image.h);

    image.xres = (int) (scale * image.xres);
    image.yres = (int) (scale * image.yres);

    scale = 256.0 / scale;

    std::cerr << "new dimensions: " << wn << " x " << hn 
	      << " (xres: " << image.xres
	      << ", yres: " << image.yres << ")" << std::endl;

    unsigned char* ndata = (unsigned char*) malloc (wn * hn);

    unsigned int offset = 0;
    for (int y=0; y < hn; y++)
      for (int x=0; x < wn; x++) {

	int bx = (int) (((double) x) * scale);
	int by = (int) (((double) y) * scale);
	
	int sx = std::min(bx / 256, image.w - 1);
	int sy = std::min(by / 256, image.h - 1);
	int sxx = std::min(sx + 1, image.w - 1);
	int syy = std::min(sy + 1, image.h - 1);

	int fxx = bx % 256;
	int fyy = by % 256;
	int fx = 256 - fxx;
	int fy = 256 - fyy;
 
	unsigned int value
	  = fx  * fy  * ( (unsigned int) image.data [sx  + image.w * sy ])
	  + fxx * fy  * ( (unsigned int) image.data [sxx + image.w * sy ])
	  + fx  * fyy * ( (unsigned int) image.data [sx  + image.w * syy])
	  + fxx * fyy * ( (unsigned int) image.data [sxx + image.w * syy]);

	value /= 256 * 256;
	value = std::min (value, (unsigned int) 255);

	ndata[offset++] = (unsigned char) value;
      }
    
    free (image.data);
    image.data = ndata;
    image.w = wn;
    image.h = hn;
  }
  
  // convert to 1-bit (threshold)
  
  unsigned char *output = image.data;
  unsigned char *input = image.data;
  
  int threshold = 170;
    
  if (arg_threshold.Get() != 0) {
    threshold = arg_threshold.Get();
    std::cerr << "Threshold: " << threshold << std::endl;
  }
    
  for (int row = 0; row < image.h; row++)

    {
      unsigned char z = 0;
      int x = 0;
      for (; x < image.w; x++)
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
  image.bps = 1;

  write_TIFF_file (arg_output.Get().c_str(), image.data, image.w, image.h,
		   image.bps, image.spp, image.xres, image.yres);
  
  return 0;
}
