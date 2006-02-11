
/*
 * Copyright (C) 2005 René Rebe
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>

#include <jasper/jasper.h>
#include <jasper/jas_image.h>

unsigned char*
read_JPEG2000_file (const char* filename, int* w, int* h, int* bps, int* spp, int* xres, int* yres)
{
  jas_image_t *image;
  jas_stream_t *in;

  if (!(in = jas_stream_fopen(filename, "rb"))) {
    fprintf(stderr, "err r: cannot open input image file %s\n", filename);
    return 0;
  }

  if (!(image = jp2_decode(in, ""))) {
    fprintf(stderr, "error: cannot load image data\n");
    return 0;
  }

  jas_stream_close (in);

  *w = jas_image_width (image);
  *h = jas_image_height (image);

#define PRINT(a,b) case a: std::cout << "Clrspc: " << a << ", " << b << std::endl; break;

  switch (jas_image_clrspc(image)) {
    PRINT(JAS_CLRSPC_UNKNOWN, "UNKNOWN")
    PRINT(JAS_CLRSPC_CIEXYZ, "CIEXYZ")
    PRINT(JAS_CLRSPC_CIELAB, "CIELAB")
    PRINT(JAS_CLRSPC_SGRAY, "SGRAY")
    PRINT(JAS_CLRSPC_SRGB, "SRGB")
    PRINT(JAS_CLRSPC_SYCBCR, "SYCBCR")
    PRINT(JAS_CLRSPC_GENGRAY, "GENRGB")
    PRINT(JAS_CLRSPC_GENRGB, "GENRGB")
    PRINT(JAS_CLRSPC_GENYCBCR, "GENYCBCR")
    default:
      std::cout << "Totally unknown colorspace ..." << std::endl;
  }

  *spp = jas_image_numcmpts(image);
  *bps = jas_image_cmptprec(image, 0/*component*/);
  if (*bps > 1 && *bps < 8) // so far we do not support 2..7 */
	*bps = 8;

  std::cout << "Components: " << jas_image_numcmpts(image)
            << ", precision: " << jas_image_cmptprec(image, 0) << std::endl;

  unsigned char* data = (unsigned char*) malloc (*h * *h * *spp);
  unsigned char* data_ptr = data;

  jas_matrix_t *jasdata[3];
  for (int k = 0; k < *spp; ++k) {
    if (!(jasdata[k] = jas_matrix_create(*h, *w))) {
      fprintf(stderr, "internal error\n");
      return 0;
    }

    if (jas_image_readcmpt(image, k, 0, 0, *w, *h, jasdata[k])) {
      fprintf(stderr, "cannot read component data %d\n", k);
      return 0;
    }
  }

  for( int y = 0; y < *h; ++y ) {
    for( int x = 0; x < *w; ++x ) {
       unsigned char v [3];
       for( int k = 0; k < *spp; ++k ) {
         v[k] = jas_matrix_get (jasdata[k], y, x);
         // if the precision of the component is too small, increase
         // it to use the complete value range.
         //v[k] <<= 8 - jas_image_cmptprec(image, jasdata[k]);
       }
#if 0
       if( ycbcr ) ycbcr_to_rgb( v, v );

       for( int k = 0; k < 3; ++k ) {
         if( v[k] < 0 ) v[k] = 0;
         else if( v[k] > 255 ) v[k] = 255;
       } // for k
#endif
       for( int k = 0; k < *spp; ++k )
       	*data_ptr++ = v[k];
    }
  }

  jas_image_destroy (image);
//  jas_stream_close (in);
  return data;
}


void
write_JPEG2000_file (const char* file, unsigned char* data, int w, int h, int bps, int spp,
                 int xres, int yres)
{
}
