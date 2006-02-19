
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


#include "utils.hh"

static void add_color_prof(jas_image_t* image)
{
  /* Create a color profile if needed. */
  if (!jas_clrspc_isunknown(image->clrspc_) &&
      !jas_clrspc_isgeneric(image->clrspc_) && !image->cmprof_) {
    if (!(image->cmprof_ =
      jas_cmprof_createfromclrspc(jas_image_clrspc(image))))
      {
        std::cout << "error: cannot create the colorspace" << std::endl;
        return;
      }
  }
}

unsigned char*
read_JPEG2000_file (const char* filename, int* w, int* h, int* bps, int* spp, int* xres, int* yres)
{
  jas_image_t *image;
  jas_stream_t *in;

  if (!(in = jas_stream_fopen(filename, "rb"))) {
    fprintf(stderr, "error: cannot open input image file %s\n", filename);
    return 0;
  }

  if (!(image = jp2_decode(in, 0))) {
    fprintf(stderr, "error: cannot load image data\n");
    return 0;
  }

  add_color_prof(image);

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
      std::cout << "Yet unknown colorspace ..." << std::endl;
  }

  // convert colorspace
  switch (jas_image_clrspc(image)) {
    case JAS_CLRSPC_SGRAY:
    case JAS_CLRSPC_SRGB:
    case JAS_CLRSPC_GENGRAY:
    case JAS_CLRSPC_GENRGB:
      break;
    default:
      {
        jas_image_t *newimage;
        jas_cmprof_t *outprof;

        std::cout << "forcing conversion to sRGB" << std::endl;
        if (!(outprof = jas_cmprof_createfromclrspc(JAS_CLRSPC_SRGB))) {
          std::cout << "cannot create sRGB profile" << std::endl;
          return 0;
        }
        std::cout << "in space: " << jas_image_cmprof(image) << std::endl;
        if (!(newimage = jas_image_chclrspc(image, outprof, JAS_CMXFORM_INTENT_PER))) {
          std::cout << "cannot convert to sRGB" << std::endl;
          return 0;
        }

        jas_image_destroy(image);
        jas_cmprof_destroy(outprof);
        image = newimage;
	std::cout << "converted to sRGB" << std::endl;
     }
  }

  *spp = jas_image_numcmpts(image);
  *bps = jas_image_cmptprec(image, 0/*component*/);
  if (*bps != 1 && *bps != 8) // we do not support the others, yet
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

  int v [3];
  for( int y = 0; y < *h; ++y ) {
    for( int x = 0; x < *w; ++x ) {
       for( int k = 0; k < *spp; ++k ) {
         v[k] = jas_matrix_get (jasdata[k], y, x);
         // if the precision of the component is not supported, scale it
         int prec = jas_image_cmptprec(image, k);
	 if (prec < 8)
           v[k] <<= 8 - prec;
	 else
	   v[k] >>= prec - 8;
       }

       for( int k = 0; k < *spp; ++k )
       	*data_ptr++ = v[k];
    }
  }

  jas_image_destroy (image);
  return data;
}


void
write_JPEG2000_file (const char* file, unsigned char* data, int w, int h, int bps, int spp,
                 int xres, int yres)
{
  jas_image_t *image;
  jas_stream_t *out;

  if (!(out = jas_stream_fopen(file, "w+b"))) {
    fprintf(stderr, "err r: cannot open output image file %s\n", file);
    return;
  }

  jas_image_cmptparm_t compparms[3];

  for (int i = 0; i < spp; ++i) {
    compparms[i].tlx = 0;
    compparms[i].tly = 0;
    compparms[i].hstep = 1;
    compparms[i].vstep = 1;
    compparms[i].width = w;
    compparms[i].height = h;
    compparms[i].prec = bps;
    compparms[i].sgnd = false;
  }

  if (!(image = jas_image_create(spp, compparms,
                                 spp==3?JAS_CLRSPC_SRGB:JAS_CLRSPC_SGRAY))) {
    std::cout << "error creating jasper image" << std::endl;
  }

  jas_matrix_t *jasdata[3];
  for (int i = 0; i < spp; ++i) {
    if (!(jasdata[i] = jas_matrix_create(h, w))) {
      fprintf(stderr, "internal error\n");
      return;
    }
  }

  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      for (int k = 0; k < spp; ++k)
        jas_matrix_set(jasdata[k], y, x, *data++);
    }
  }

  for (int i = 0; i < spp; ++i) {
    int ct = JAS_IMAGE_CT_GRAY_Y;
    if (spp > 1)
      switch (i) {
       case 0: ct = JAS_IMAGE_CT_RGB_R; break;
       case 1: ct = JAS_IMAGE_CT_RGB_G; break;
       case 2: ct = JAS_IMAGE_CT_RGB_B; break;
    }
    jas_image_setcmpttype (image, i, ct );
    if (jas_image_writecmpt(image, i, 0, 0, w, h, jasdata[i])) {
      std::cout << "error writing converted data into jasper" << std::endl;
      return;
    }
  }

  jp2_encode(image, out, 0);
  jas_image_destroy (image);
  jas_stream_close (out);
}
