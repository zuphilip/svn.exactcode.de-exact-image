
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <tiffconf.h>
#include <tiffio.h>

#include "tiff.hh"

#include "Colorspace.hh"

bool TIFFLoader::readImage (const char *file, Image& image)
{
  TIFF* in;
  in = TIFFOpen(file, "r");
  if (!in)
    return false;
  
  uint16 photometric = 0;
  TIFFGetField(in, TIFFTAG_PHOTOMETRIC, &photometric);
  //printf ("photometric: %d\n", photometric);
  switch (photometric)
    {
    case PHOTOMETRIC_MINISWHITE:
    case PHOTOMETRIC_MINISBLACK:
    case PHOTOMETRIC_RGB:
    case PHOTOMETRIC_PALETTE:
      break;
    
    default:
      printf("Bad photometric: %d\n", photometric);
      return 0;
    }
  
  uint32 _w;
  TIFFGetField(in, TIFFTAG_IMAGEWIDTH, &_w); image.w = _w;
  
  uint32 _h;
  TIFFGetField(in, TIFFTAG_IMAGELENGTH, &_h); image.h = _h;
  
  uint16 _spp;
  TIFFGetField(in, TIFFTAG_SAMPLESPERPIXEL, &_spp); image.spp = _spp;
  
  uint16 _bps;
  TIFFGetField(in, TIFFTAG_BITSPERSAMPLE, &_bps); image.bps = _bps;
  
  uint16 config;
  TIFFGetField(in, TIFFTAG_PLANARCONFIG, &config);
  
  uint16 _xres;
  if (TIFFGetField(in, TIFFTAG_XRESOLUTION, &_xres) == 0)
    image.xres = _xres;
  
  uint16 _yres;
  if (TIFFGetField(in, TIFFTAG_YRESOLUTION, &_yres) == 0)
    image.yres = _yres;
  
  int stride = image.Stride();

  // printf ("w: %d h: %d\n", *w, *h);
  // printf ("spp: %d bps: %d stride: %d\n", *spp, *bps, stride);

  image.data = (unsigned char* ) malloc (stride * image.h);
  
  uint16 *rmap = 0, *gmap = 0, *bmap = 0;
  if (photometric == PHOTOMETRIC_PALETTE)
    {
      if (!TIFFGetField (in, TIFFTAG_COLORMAP, &rmap, &gmap, &bmap))
	printf ("Error reading colormap.\n");
    }

  unsigned char* data2 = image.data;
  for (int row = 0; row < image.h; row++)
    {
      if (TIFFReadScanline(in, data2, row, 0) < 0)
	break;
      // invert if saved inverted
      if (photometric == PHOTOMETRIC_MINISWHITE && image.bps == 1)
	for (unsigned char* i = data2; i != data2 + stride; ++i)
	  *i ^= 0xFF;

      data2 += stride;
    }
  
  /* strange 16bit gray images ??? */
  if (image.spp == 2)
    {
      for (unsigned char* it = image.data;
	   it < image.data + stride * image.h; it += 2) {
	char x = it[0];
	it[0] = it[1];
	it[1] = x;
      }
      
      image.spp = 1;
      image.bps *= 2;
    }
  
  if (photometric == PHOTOMETRIC_PALETTE) {
    colorspace_de_palette (image, rmap, gmap, bmap);
    free (rmap);
    free (gmap);
    free (bmap);
  }
  
  return true;
}

bool TIFFLoader::writeImage (const char *file, Image& image)
{
  TIFF *out;
  //char thing[1024];
  unsigned char /* *inbuf,*/  *outbuf;

  uint32 rowsperstrip = (uint32) - 1;
  uint16 compression = COMPRESSION_NONE;
  if (image.bps == 1)
    compression = COMPRESSION_CCITTFAX4;
  else
    compression = COMPRESSION_LZW;

  out = TIFFOpen (file, "w");
  if (out == NULL)
    return false;
   
  TIFFSetField (out, TIFFTAG_IMAGEWIDTH, image.w);
  TIFFSetField (out, TIFFTAG_IMAGELENGTH, image.h);
  TIFFSetField (out, TIFFTAG_BITSPERSAMPLE, image.bps);
  TIFFSetField (out, TIFFTAG_SAMPLESPERPIXEL, image.spp);
  TIFFSetField (out, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);

  TIFFSetField (out, TIFFTAG_COMPRESSION, compression);
  if (image.bps == 1)
    TIFFSetField (out, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
  else if (image.spp == 1) {
    uint16 rmap[256], gmap[256], bmap[256];
    int i;
    for (i=0;i<256;++i) {
      rmap[i] = gmap[i] = bmap[i] = (i<<8);
    }
    TIFFSetField (out, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_PALETTE);
    TIFFSetField (out, TIFFTAG_COLORMAP, rmap, gmap, bmap);
  }
  else
    TIFFSetField (out, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
  
  if (image.xres != 0)
    TIFFSetField (out, TIFFTAG_XRESOLUTION, image.xres);
  
  if (image.yres != 0)
    TIFFSetField (out, TIFFTAG_YRESOLUTION, image.yres);
  
  TIFFSetField (out, TIFFTAG_IMAGEDESCRIPTION, "none");
  TIFFSetField (out, TIFFTAG_SOFTWARE, "ExactImage");
  outbuf = (unsigned char *) _TIFFmalloc (TIFFScanlineSize (out));
  TIFFSetField (out, TIFFTAG_ROWSPERSTRIP,
		TIFFDefaultStripSize (out, rowsperstrip));

  int stride = image.Stride();
  for (int row = 0; row < image.h; row++)
    {
      if (TIFFWriteScanline (out, image.data + row * stride, row, 0) < 0)
	break;
    }
  
  TIFFClose (out);
  
  return true;
}

TIFFLoader tiff_loader;
