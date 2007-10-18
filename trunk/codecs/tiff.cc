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
#include <tiffio.hxx>

#include "tiff.hh"

#include "Colorspace.hh"

#include <algorithm>
#include <iostream>

bool TIFCodec::readImage (std::istream* stream, Image& image)
{
  TIFF* in;
  
  // quick magic check
  {
    char a, b;
    a = stream->get ();
    b = stream->peek ();
    stream->putback (a);
    
    int magic = (a << 8) | b;
    
    if (magic != TIFF_BIGENDIAN && magic != TIFF_LITTLEENDIAN)
      return false;
  }
  
  in = TIFFStreamOpen ("", stream);
  if (!in)
    return false;
  
  uint16 photometric = 0;
  TIFFGetField(in, TIFFTAG_PHOTOMETRIC, &photometric);
  // std::cerr << "photometric: " << (int)photometric << std::endl;
  switch (photometric)
    {
    case PHOTOMETRIC_MINISWHITE:
    case PHOTOMETRIC_MINISBLACK:
    case PHOTOMETRIC_RGB:
    case PHOTOMETRIC_PALETTE:
      break;
    default:
      std::cerr << "TIFCodec: Unrecognized photometric: " << (int)photometric << std::endl;
      return false;
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
  
  float _xres;
  if (TIFFGetField(in, TIFFTAG_XRESOLUTION, &_xres))
    image.xres = (int)_xres;
  
  float _yres;
  if (TIFFGetField(in, TIFFTAG_YRESOLUTION, &_yres))
    image.yres = (int)_yres;
  
  int stride = image.stride();

  // printf ("w: %d h: %d\n", _w, _h);
  // printf ("spp: %d bps: %d stride: %d\n", _spp, _bps, stride);

  image.New (image.w, image.h);
  
  uint16 *rmap = 0, *gmap = 0, *bmap = 0;
  if (photometric == PHOTOMETRIC_PALETTE)
    {
      if (!TIFFGetField (in, TIFFTAG_COLORMAP, &rmap, &gmap, &bmap))
	std::cerr << "TIFCodec: Error reading colormap." << std::endl;
    }

  uint8_t* data2 = image.getRawData();
  for (int row = 0; row < image.h; row++)
    {
      if (TIFFReadScanline(in, data2, row, 0) < 0)
	break;
      data2 += stride;
    }
  
  /* some post load fixup */
  
  /* invert if saved "inverted" */
  if (photometric == PHOTOMETRIC_MINISWHITE)
    invert (image);
  
  /* strange 16bit gray images ??? or GRAYA? */
  if (image.spp == 2)
    {
      for (uint8_t* it = image.getRawData();
	   it < image.getRawDataEnd(); it += 2) {
	char x = it[0];
	it[0] = it[1];
	it[1] = x;
      }
      
      image.spp = 1;
      image.bps *= 2;
    }
  
  if (photometric == PHOTOMETRIC_PALETTE) {
    colorspace_de_palette (image, 1 << image.bps, rmap, gmap, bmap);
    /*    free (rmap);
	  free (gmap);
	  free (bmap); */
  }

  TIFFClose (in);
  return true;
}

bool TIFCodec::writeImage (std::ostream* stream, Image& image, int quality,
			   const std::string& compress)
{
  TIFF* out;
  
  out = TIFFStreamOpen ("", stream);
  if (out == NULL)
    return false;
  
  writeImageImpl (out, image, compress, 0);

  TIFFClose (out);
  
  return true;
}

bool TIFCodec::writeImageImpl (TIFF* out, const Image& image, const std::string& compress,
			       int page)
{
  uint32 rowsperstrip = (uint32) - 1;
  
  uint16 compression = COMPRESSION_NONE;
  if (compress.empty()) {
    if (image.bps == 1)
      compression = COMPRESSION_CCITTFAX4;
    else
      compression = COMPRESSION_DEFLATE;
  }
  else {
    std::string c (compress);
    std::transform (c.begin(), c.end(), c.begin(), tolower);
    
    if (c == "g3" || c == "fax")
      compression = COMPRESSION_CCITTFAX3;
    else if (c == "g4" || c == "group4")
      compression = COMPRESSION_CCITTFAX4;
    else if (c == "lzw")
      compression = COMPRESSION_LZW;
    else if (c == "deflate" || c == "zip")
      compression = COMPRESSION_DEFLATE;
    else if (c == "none")
      compression = COMPRESSION_NONE;
    else
      std::cerr << "TIFCodec: Unrecognized compression option '" << compress << "'" << std::endl;
  }
  
  if (page) {
    TIFFSetField (out, TIFFTAG_SUBFILETYPE, FILETYPE_PAGE);
    TIFFSetField (out, TIFFTAG_PAGENUMBER, page, 0); // total number unknown
    // TIFFSetField (out, TIFFTAG_PAGENAME, page_name);
  }
  
  TIFFSetField (out, TIFFTAG_IMAGEWIDTH, image.w);
  TIFFSetField (out, TIFFTAG_IMAGELENGTH, image.h);
  TIFFSetField (out, TIFFTAG_BITSPERSAMPLE, image.bps);
  TIFFSetField (out, TIFFTAG_SAMPLESPERPIXEL, image.spp);
  TIFFSetField (out, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);

  TIFFSetField (out, TIFFTAG_COMPRESSION, compression);
  if (image.spp == 1 && image.bps == 1)
    // internally we actually have MINISBLACK, but some programs,
    // including the Apple Preview.app appear to ignore this bit
    TIFFSetField (out, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISWHITE);
  else if (image.spp == 1)
    TIFFSetField (out, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
  else if (false) { /* just saved for reference */
    uint16 rmap[256], gmap[256], bmap[256];
    for (int i = 0;i < 256; ++i) {
      rmap[i] = gmap[i] = bmap[i] = i * 0xffff / 255;
    }
    TIFFSetField (out, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_PALETTE);
    TIFFSetField (out, TIFFTAG_COLORMAP, rmap, gmap, bmap);
  }
  else
    TIFFSetField (out, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
  
  if (image.xres != 0) {
    float _xres = image.yres;
    TIFFSetField (out, TIFFTAG_XRESOLUTION, _xres);
  }
  
  if (image.yres != 0) {
    float _yres = image.yres;
    TIFFSetField (out, TIFFTAG_YRESOLUTION, _yres);
  }
  
  TIFFSetField (out, TIFFTAG_IMAGEDESCRIPTION, "none");
  TIFFSetField (out, TIFFTAG_SOFTWARE, "ExactImage");
  TIFFSetField (out, TIFFTAG_ROWSPERSTRIP,
		TIFFDefaultStripSize (out, rowsperstrip));

  const int stride = image.stride();
  /* Note: we on-the-fly invert 1-bit data to please some historic apps */
  
  uint8_t* src = image.getRawData();
  uint8_t* scanline = 0;
  if (image.bps == 1)
    scanline = (uint8_t*) malloc (stride);
  
  for (int row = 0; row < image.h; ++row, src += stride) {
    int err = 0;
    if (image.bps == 1) {
      for (int i = 0; i < stride; ++i)
        scanline [i] = src [i] ^ 0xFF;
      err = TIFFWriteScanline (out, scanline, row, 0);
    }
    else
      err = TIFFWriteScanline (out, src, row, 0);
    
    if (err < 0) {
      if (scanline) free (scanline);
      return false;
    }
  }
  if (scanline) free (scanline);  
  
  TIFFWriteDirectory (out);
  
  return true;
}

TIFCodec tif_loader;
