
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


unsigned char*
TIFFLoader::readImage (const char *file, int* w, int* h, int* bps, int* spp,
		       int* xres, int* yres)
{
  TIFF* in;
  in = TIFFOpen(file, "r");
  if (!in)
    return 0;
  
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
  
  TIFFGetField(in, TIFFTAG_IMAGEWIDTH, w);
  TIFFGetField(in, TIFFTAG_IMAGELENGTH, h);
  
  uint16 _spp;
  TIFFGetField(in, TIFFTAG_SAMPLESPERPIXEL, &_spp); *spp = _spp;
  
  uint16 _bps;
  TIFFGetField(in, TIFFTAG_BITSPERSAMPLE, &_bps); *bps = _bps;
  
  uint16 config;
  TIFFGetField(in, TIFFTAG_PLANARCONFIG, &config);
  
  uint16 _xres;
  if (TIFFGetField(in, TIFFTAG_XRESOLUTION, &_xres) == 0)
    *xres = _xres;
  
  uint16 _yres;
  if (TIFFGetField(in, TIFFTAG_YRESOLUTION, &_yres) == 0)
    *yres = _yres;
  
  int stride = (((*w) * (*spp) * (*bps)) + 7) / 8;

  //  printf ("w: %d h: %d\n", *w, *h);
  //  printf ("spp: %d bps: %d stride: %d\n", *spp, *bps, stride);

  unsigned char* data = (unsigned char* ) malloc (stride * *h);
  
  unsigned char* data2 = data;
  for (unsigned int row = 0; row < *h; row++)
    {
      if (TIFFReadScanline(in, data2, row, 0) < 0)
	break;
      // invert if saved inverted
      if (photometric == PHOTOMETRIC_MINISWHITE && *bps == 1)
	for (unsigned char* i = data2; i != data2 + stride; ++i)
	  *i ^= 0xFF;

      data2 += stride;
    }
  return data;
}

void
TIFFLoader::writeImage (const char *file, unsigned char *data, int w, int h,
			int bps, int spp, int xres, int yres)
{
  TIFF *out;
  //char thing[1024];
  unsigned char /* *inbuf,*/  *outbuf;

  uint32 rowsperstrip = (uint32) - 1;
  uint16 compression = COMPRESSION_NONE;
  if (bps == 1)
    compression = COMPRESSION_CCITTFAX4;
  else
    compression = COMPRESSION_LZW;

  out = TIFFOpen (file, "w");
  if (out == NULL)
    return;
  TIFFSetField (out, TIFFTAG_IMAGEWIDTH, w);
  TIFFSetField (out, TIFFTAG_IMAGELENGTH, h);
  TIFFSetField (out, TIFFTAG_BITSPERSAMPLE, bps);
  TIFFSetField (out, TIFFTAG_SAMPLESPERPIXEL, spp);
  TIFFSetField (out, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);

  TIFFSetField (out, TIFFTAG_COMPRESSION, compression);
  if (bps == 1)
    TIFFSetField (out, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
  else if (spp == 1) {
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
  
  if (xres != 0)
    TIFFSetField (out, TIFFTAG_XRESOLUTION, xres);
  
  if (yres != 0)
    TIFFSetField (out, TIFFTAG_YRESOLUTION, yres);
  
  TIFFSetField (out, TIFFTAG_IMAGEDESCRIPTION, "none");
  TIFFSetField (out, TIFFTAG_SOFTWARE, "ExactImage");
  outbuf = (unsigned char *) _TIFFmalloc (TIFFScanlineSize (out));
  TIFFSetField (out, TIFFTAG_ROWSPERSTRIP,
		TIFFDefaultStripSize (out, rowsperstrip));
  
  for (uint32 row = 0; row < h; row++)
    {
      if (TIFFWriteScanline (out, data, row, 0) < 0)
	break;
      data += (w * spp * bps + 7) / 8;
    }

  TIFFClose (out);
}

TIFFLoader tiff_loader;
