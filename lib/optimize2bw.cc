
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

#include "Image.hh"
#include "Codecs.hh"

#include "Colorspace.hh"
#include "Matrix.hh"
#include "scale.hh"

#include "optimize2bw.hh"

void optimize2bw (Image& image, int low, int high, int threshold,
		  int sloppy_threshold,
		  int radius, double standard_deviation)
{
  /* Convert to RGB to gray.
     If the threshold is to be determined automatically, use color info. */
  
  const int debug = 0;
  
  if (threshold) { // simple normalize
    if (image.spp == 3 && image.bps == 8)
      colorspace_rgb8_to_gray8 (image);
    
    if (image.spp != 1 && image.bps != 8) {
      std::cerr << "Can't yet handle " << image.spp << " samples with "
		<< image.bps << " bits per sample." << std::endl;
      return;
    }
    
    // CARE: if the normalize API changse, care must be taken to
    //       adapt this to honor whether low and high actually have
    //       been supplied
    
    normalize (image, low, high);
  }
  else { // color normalize on background color
    
    // search for background color */
    int histogram[256][3] = { {0}, {0} };
    
    for (Image::iterator it = image.begin(); it != image.end(); ++it) {
      uint16_t r, g, b;
      *it;
      it.getRGB (&r, &g, &b);
      
      histogram[r][0]++;
      histogram[g][1]++;
      histogram[b][2]++;
    }
    
    int lowest = 255, highest = 0, bg_r = 0, bg_g = 0, bg_b = 0;
    for (int i = 0; i <= 255; i++)
      {
	int r, g, b;
	r = histogram[i][0];
	g = histogram[i][1];
	b = histogram[i][2];
	
	if (debug)
	  std::cout << i << ": "<< r << " " << g << " " << b << std::endl;
	const int magic = 2; // magic denoise constant
	if (r >= magic || g >= magic || b >= magic)
	  {
	    if (i < lowest)
	      lowest = i;
	    if (i > highest)
	      highest = i;
	  }
	
	if (histogram[i][0] > histogram[bg_r][0])
	  bg_r = i;
	if (histogram[i][1] > histogram[bg_g][1])
	  bg_g = i;
	if (histogram[i][2] > histogram[bg_b][2])
	  bg_b = i;
      }
    std::cerr << "lowest: " << lowest << ", highest: " << highest
	      << ", back rgb: " << bg_r << " " <<  bg_g << " " << bg_b << std::endl;
    
    highest = (.21267 * bg_r + .71516 * bg_g + .07217 * bg_b);
    
    if (low)
      lowest = low;
    if (high)
      highest = high;
    
    signed int a = (255 * 256) / (highest - lowest);
    signed int b = (-a * lowest);
    
    std::cerr << "a: " << (float) a / 256
	      << " b: " << (float) b / 256 << std::endl;
    
    for (Image::iterator it = image.begin(); it != image.end (); ++it) {
      *it;
      // TODO: under/overflow!
      Image::iterator i = (it * a + b) / 256;
      i.limit();
      it.set(i);
    }
    
    if (image.spp == 3 && image.bps == 8)
      colorspace_rgb8_to_gray8 (image);
    
    if (image.spp != 1 && image.bps != 8) {
      std::cerr << "Can't yet handle " << image.spp << " samples with "
		<< image.bps << " bits per sample." << std::endl;
      return;
    }
  }
  
  
  // Convolution Matrix (unsharp mask a-like)
  {
    // compute kernel (convolution matrix to move over the iamge)
    
    const int width = radius * 2 + 1;
    matrix_type divisor = 0;
    float sd = standard_deviation;
    
    matrix_type* matrix = new matrix_type [width * width];
    
    if (debug)
      std::cerr << std::fixed << std::setprecision(3);
    for (int y = -radius; y <= radius; ++y) {
      for (int x = -radius; x <= radius; ++x) {
	matrix_type v = (matrix_type) (exp (-((float)x*x + (float)y*y) / (2. * sd * sd)) * 5);
	divisor += v;
	
	matrix[x + radius + (y+radius)*width] = v;
      }
    }
    
    // sub from image *2 and print
    for (int y = -radius; y <= radius; ++y) {
      for (int x = -radius; x <= radius; ++x) {
	matrix_type* m = &matrix[x + radius + (y+radius)*width];
	
	*m *= -1;
	if (x == 0 && y == 0)
	  *m += 2*divisor;
	
	if (debug)
	  std::cerr << *m << " ";
      }
      if (debug)
	std::cerr << std::endl;
    }
    
    const int sloppy_thr = sloppy_threshold;
    
    convolution_matrix (image, matrix, width, width, divisor);
  }
}
