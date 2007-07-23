/*
 * Copyright (C) 2005 - 2007 René Rebe
 *           (C) 2005 - 2007 Archivista GmbH, CH-8042 Zuerich
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

/* Short Description:
 *   Any color-space to b/w (1-bit) optimization. Applications involving
 *   huge amount of document archiving often prefer storing the data
 *   as (compressed) b/w data even in the age of TB storage as higher
 *   (color) image often explores the data volume exponentially.
 *
 *   For this case this algorithm automatically determines a threshold
 *   and performs some optimizations in order to loose as few details
 *   as possible (e.g. retrieve hand notes, stamps, etc. but leave
 *   the background color, coffee sparkels, dust and folding lines
 *   out.
 *
 *   While the first version only performed a unsharp mask and auto-
 *   thresholding, the latest version does perform a segmentation-pass,
 *   to optimize the performance (by leaving out empty areas) and
 *   to keep dithering of image areas.
 */

#include <math.h>

#include <iostream>
#include <iomanip>

#include "Image.hh"

#include "Colorspace.hh"
#include "Matrix.hh"

#include "optimize2bw.hh"

/* private copy of the convolution_matrix, checking for edges at the
   center of the destination pixel in order to skip the expensive
   matrix convolution and thus improve performance */

static void private_convolution_matrix (Image& image, matrix_type* matrix, int xw, int yw,
					matrix_type divisor)
{
  uint8_t* data = image.getRawData();
  uint8_t* new_data = (uint8_t*) malloc (image.w * image.h);
  
  const int xr = xw / 2;
  const int yr = yw / 2;

  const int _y1 = std::min (image.h, yr);
  const int _y2 = std::max (image.h-yr, yr);

  uint8_t* src_ptr = data;
  uint8_t* dst_ptr = new_data;

  const int kernel_offset = yr * image.w + xr;

  // top-border
  for (int i = 0; i < _y1 * image.w; ++i)
    *dst_ptr++ = *src_ptr++;
  
  divisor = 1 / divisor; // to multiple in the loop
  
  for (int y = _y1; y < _y2; ++y)
  {
    src_ptr = &data[y * image.w];
    dst_ptr = &new_data[y * image.w];

    // left-side border
    for (int x = 0; x < xr; ++x)
        *dst_ptr++ = *src_ptr++;

    // convolution area
    for (int x = xr; x < image.w-xr; ++x)
      {
	  uint8_t* data_row = src_ptr++ - kernel_offset;
	  
	  // if no local edge, copy pixel and exit early
	  {
	    uint8_t* it = data_row + kernel_offset;
	    uint8_t ref = *it;
	    
	    int diff = std::abs ( (int)it[-1-image.w] - ref ) +
	      std::abs ( (int)it[+1-image.w] - ref ) +
	      std::abs ( (int)it[-1+image.w] - ref ) +
	      std::abs ( (int)it[+1+image.w] - ref );
	    
	    if (diff < 4*4) {
	      *dst_ptr++ = ref;
	      continue;
	    }
	  }
	  
	  matrix_type sum1 = 0, sum2 = 0, sum3 = 0, sum4 = 0;

	  matrix_type* matrix_row = matrix;
	  // in former times this was more readable and got overoptimized
	  // for speed ,-)
	  for (int y2 = 0; y2 < yw; ++y2, data_row += image.w - xw) {
	    int x2 = xw;
	    while (x2 >= 4) {
	      sum1 += matrix_row[0] * data_row[0];
	      sum2 += matrix_row[1] * data_row[1];
	      sum3 += matrix_row[2] * data_row[2];
	      sum4 += matrix_row[3] * data_row[3];
	      data_row += 4;
	      matrix_row += 4;
	      x2 -= 4;
	    }
	    while (x2 > 0) {
 	      sum1 += *matrix_row++ * *data_row++;
	      --x2;
	    }
	  }
	  
	  matrix_type sum = (sum1+sum2+sum3+sum4) * divisor;
	  uint8_t z = (uint8_t) (sum > 255 ? 255 : sum < 0 ? 0 : sum);
	  *dst_ptr++ = z;
      }
    // right-side border
    for (int x = image.w-xr; x < image.w; ++x)
        *dst_ptr++ = *src_ptr++;
  }

  // bottom-border
  for (int i = 0; i < image.w * (image.h-_y2); ++i)
    *dst_ptr++ = *src_ptr++;

  image.setRawData (new_data);
}


void optimize2bw (Image& image, int low, int high, int threshold,
		  int sloppy_threshold,
		  int radius, double standard_deviation)
{
  // do nothing if already at s/w, ...
  if (image.spp == 1 && image.bps == 1)
    return;
  
  /* Convert to RGB to gray.
     If the threshold is to be determined automatically, use color info. */
  
  const bool debug = false;
  
  // color normalize on background color
  // search for background color */
  {
    int histogram[256][3] = { {0}, {0} };
    
    colorspace_by_name (image, "rgb8");
    
    uint8_t* it = image.getRawData ();
    uint8_t* end = image.getRawDataEnd ();
    
    while (it != end) {
      uint8_t r = *it++;
      uint8_t g = *it++;
      uint8_t b = *it++;
      histogram[r][0]++;
      histogram[g][1]++;
      histogram[b][2]++;
    }
    
    int lowest = 255, highest = 0, bg_r = 0, bg_g = 0, bg_b = 0;
    for (int i = 0; i <= 255; i++)
      {
	int r = histogram[i][0];
	int g = histogram[i][1];
	int b = histogram[i][2];
	
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
    highest = (int) (.21267 * bg_r + .71516 * bg_g + .07217 * bg_b);
        
    std::cerr << "lowest: " << lowest << ", highest: " << highest
	      << ", back rgb: " << bg_r << " " <<  bg_g << " " << bg_b
	      << std::endl;
    
    const int min_delta = 128;
    lowest = std::max (std::min (lowest, highest - min_delta), 0);
    highest = std::min (std::max (highest, lowest + min_delta), 255);
    
    if (low)
      lowest = low;
    if (high)
      highest = high;

    std::cerr << "after limit and overwrite, lowest: " << lowest
	      << ", highest: " << highest << std::endl;
    
    signed int a = (255 * 256) / (highest - lowest);
    signed int b = (-a * lowest);
    
    std::cerr << "a: " << (float) a / 256
	      << " b: " << (float) b / 256 << std::endl;

    it = image.getRawData ();
    end = image.getRawDataEnd ();
    uint8_t*it2 = it;
    
    while (it != end) {
      int _r = *it++;
      int _g = *it++;
      int _b = *it++;
      
      _r = (_r * a + b) / 256;
      _g = (_g * a + b) / 256;
      _b = (_b * a + b) / 256;
      
      // clip
      _r = std::max (std::min (_r, 255), 0);
      _g = std::max (std::min (_g, 255), 0);
      _b = std::max (std::min (_b, 255), 0);
      
      // on-the-fly convert to gray with associated weighting
      *it2++ = (_r*28 + _g*59 + _b*11) / 100;
    }
    
    image.spp = 1; // converted data RGB8->GRAY8
    image.setRawData();
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
    
    private_convolution_matrix (image, matrix, width, width, divisor);
  }
}

void differential_optimize2bw (Image& image,
			       int low, int high, int threshold,
			       int sloppy_threshold,
			       int radius, double standard_deviation)
{
  // do nothing if already at s/w, ...
  if (image.spp == 1 && image.bps == 1)
    return;
  
  colorspace_by_name (image, "gray8");
  
  Image src_image (image);
  uint8_t* it = src_image.getRawData ();
  uint8_t* dst = image.getRawData ();

  for (unsigned int y = 0; y < image.h; ++y)
    for (unsigned int x = 0; x < image.w; ++x, ++it, ++dst)
      {
	if (x > 0 && y > 0 &&
	    x < image.w-1 && y < image.h-1)
	  {
	    const int ref = it[0];
	    unsigned int sum =
	      std::abs ( (int)it[-1] - ref ) +
	      std::abs ( (int)it[+1] - ref ) +
	      std::abs ( (int)it[-image.w] - ref ) +
	      std::abs ( (int)it[+image.w] - ref ) +
	      
	      (
	       std::abs ( (int)it[-1-image.w] - ref ) +
	      std::abs ( (int)it[+1-image.w] - ref ) +
	      std::abs ( (int)it[-1+image.w] - ref ) +
	      std::abs ( (int)it[+1+image.w] - ref )
	       ) / 2
	      ;
	    
	    if (sum > 255)
	      sum = 255;
	    sum = 255-sum;
	    
	    dst[0] = sum > 127 ? 255 : 0;
	  }
      }
  image.setRawData ();
}
