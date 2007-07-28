/*
 * Convolution Matrix.
 * Copyright (C) 2006, 2007 René Rebe, ExactCODE
 * Copyright (C) 2007 Valentin Ziegler, ExactCODE
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
 */

#include "Matrix.hh"

void convolution_matrix (Image& image, const matrix_type* matrix, int xw, int yw,
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
	  matrix_type sum1 = 0, sum2 = 0, sum3 = 0, sum4 = 0;

	  const matrix_type* matrix_row = matrix;
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


void decomposable_convolution_matrix (Image& image, const matrix_type* h_matrix, const matrix_type* v_matrix, int xw, int yw,
				      matrix_type src_add)
{
  uint8_t* data = image.getRawData();
  matrix_type* tmp_data = (matrix_type*) malloc (image.w * image.h*sizeof(matrix_type));
  
  const int xr = xw / 2;
  const int yr = yw / 2;
  const int xmax = image.w-((xw+1)/2);
  const int ymax = image.h-((yw+1)/2);

  uint8_t* src_ptr = data;
  matrix_type* tmp_ptr = tmp_data;

  // transform the vertical convolution strip with h_matrix, leaving out the left/right borders
  for (int y = 0; y < image.h; ++y) {
    //printf("%d\n",y);
    src_ptr = &data[(y * image.w)-xr];
    tmp_ptr = &tmp_data[y * image.w];

    for (int x = xr; x < xmax; ++x) {
      tmp_ptr[x]=.0;
      for (int dx = 0; dx < xw; ++dx)
	tmp_ptr[x]+=((matrix_type)src_ptr[x+dx])*h_matrix[dx];
    }
  }

  // transform the horizontal convolution strip with v_matrix, leaving out all borders
  for (int x = xr; x < xmax; ++x) {

    src_ptr = &data[x+(yr*image.w)];
    tmp_ptr = &tmp_data[x];

    int offs=0;
    for (int y = yr; y < ymax; ++y) {
      int doffs=offs;
      matrix_type sum=src_add * ((matrix_type) src_ptr[offs]);
      for (int dy = 0; dy < yw; ++dy) {
	sum += tmp_ptr[doffs]*v_matrix[dy];
	doffs+=image.w;
      }
      uint8_t z = (uint8_t) (sum > 255 ? 255 : sum < 0 ? 0 : sum);
      src_ptr[offs]=z;
      offs+=image.w;
    }
  }

  image.setRawData (); // invalidate as altered

  free (tmp_data);
}
