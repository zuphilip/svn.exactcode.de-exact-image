/*
 * Convolution Matrix.
 * Copyright (C) 2006, 2007 René Rebe
 * Copyright (C) 2006 Archivista
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

#ifndef MATRIX_HH
#define MATRIX_HH

// any matrix and devisior
// on my AMD Turion speed is: double > int > float
// and in 32bit mode: double > float > int ?
typedef double matrix_type;

inline void convolution_matrix (Image& image, matrix_type* matrix, int xw, int yw,
			 matrix_type divisor)
{
  uint8_t* data = image.getRawData();
  uint8_t* new_data = (uint8_t*) malloc (image.w * image.h);
  
  int xr = xw / 2;
  int yr = yw / 2;

  for (int y = 0; y < image.h; ++y)
  {
    uint8_t* src_ptr = &data[y * image.w];
    uint8_t* dst_ptr = &new_data[y * image.w];

    for (int x = 0; x < image.w; ++x)
      {
	const uint8_t val = *src_ptr++;
	
	// for now, copy border pixels
	if (y < yr || y > image.h - yr ||
	    x < xr || x > image.w - xr)
	  *dst_ptr++ = val;
	else {
	  matrix_type sum = 0;
	  uint8_t* data_row =
	    &data[ (y - yr) * image.w - xr + x];
	  matrix_type* matrix_row = matrix;
	  // in former times this was more readable and got overoptimized
	  // for speed ,-)
	  for (int y2 = 0; y2 < yw; ++y2, data_row += image.w - xw)
	    for (int x2 = 0; x2 < xw; ++x2)
	      sum += *data_row++ * *matrix_row++;
	  
	  sum /= divisor;
	  uint8_t z = (uint8_t) (sum > 255 ? 255 : sum < 0 ? 0 : sum);
	  *dst_ptr++ = z;
	}
      }
  }
  
  image.setRawData (new_data);
}

#endif
