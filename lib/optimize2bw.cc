
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

void optimize2bw (Image& image, int low, int high,
		  int threshold, int sloppy_threshold,
		  int radius, double standard_deviation)
{
  // convert to RGB to gray - TODO: more cases
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
  
  // Convolution Matrix (unsharp mask a-like)
  {
    // compute kernel (convolution matrix to move over the iamge)
    
    const int width = radius * 2 + 1;
    matrix_type divisor = 0;
    float sd = standard_deviation;
    
    matrix_type* matrix = new matrix_type [width * width];
    
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
	
	std::cerr << *m << " ";
      }
      std::cerr << std::endl;
    }
    std::cerr << "Divisor: " << divisor << std::endl;

    const int sloppy_thr = sloppy_threshold;
    std::cout << "Lazy threshold: " << sloppy_thr << std::endl;
    
    convolution_matrix (image, matrix, width, width, divisor);
  }
  
  // convert to 1-bit (threshold)
  colorspace_gray8_to_gray1 (image, threshold);
}
