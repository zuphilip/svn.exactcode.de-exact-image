/*
 * C++ PCX library.
 * Copyright (C) 2008 René Rebe, ExactCODE GmbH Germany
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
 * Alternatively, commercial licensing options are available from the
 * copyright holder ExactCODE GmbH Germany.
 *
 */

#include <iostream>

#include <string.h>
#include <ctype.h>

#include "pcx.hh"

#include "Colorspace.hh"
#include "Endianess.hh"
#include "inttypes.h"

using Exact::EndianessConverter;
using Exact::LittleEndianTraits;

typedef struct
{
  uint8_t Manufacturer;
  uint8_t Version;
  uint8_t Encoding;
  uint8_t BitsPerPixel;
  
  EndianessConverter<uint16_t,LittleEndianTraits> WindowXmin;
  EndianessConverter<uint16_t,LittleEndianTraits> WindowYmin;
  
  EndianessConverter<uint16_t,LittleEndianTraits> WindowXmax;
  EndianessConverter<uint16_t,LittleEndianTraits> WindowYmax;
  
  EndianessConverter<uint16_t,LittleEndianTraits> HDpi;
  EndianessConverter<uint16_t,LittleEndianTraits> VDpi;
  
  uint8_t Colormap[48];
  uint8_t Reserved;
  
  uint8_t NPlanes;
  
  EndianessConverter<uint16_t,LittleEndianTraits> BytesPerLine;
  EndianessConverter<uint16_t,LittleEndianTraits> PaletteInfo;

  EndianessConverter<uint16_t,LittleEndianTraits> HScreenSize;
  EndianessConverter<uint16_t,LittleEndianTraits> VScreenSize;

  uint8_t Filler[54];
} __attribute__((packed)) PCXHeader;

bool PCXCodec::readImage(std::istream* stream, Image& image, const std::string& decompres)
{
  PCXHeader header;
  
  if (stream->peek() == 10) {
    header.Manufacturer = stream->get();
    if (stream->peek() > 5) {
      stream->unget();
      return false;
    }
    stream->unget();
  }
  
  if (!stream->read((char*)&header, sizeof(header)))
    return false;
  
  std::cerr << sizeof(header) << std::endl;
  
  image.bps = header.BitsPerPixel;
  image.spp = header.NPlanes;
  image.xres = header.HDpi;
  image.yres = header.VDpi;
  
  image.resize(header.WindowXmax - header.WindowXmin + 1,
	       header.WindowYmax - header.WindowYmin + 1);
  
  std::cerr << image.w << "x" << image.h << std::endl;
  std::cerr << "Version: " << (int)header.Version
	    << ", PaletteInfo: " << header.PaletteInfo << std::endl;
  std::cerr << "BitesPerPixel: " << (int)header.BitsPerPixel
	    << ", NPlanes: " << (int)header.NPlanes << std::endl;
  std::cerr << "BytesPerLine: " << (int)header.BytesPerLine << std::endl;
  std::cerr << "Encoding: " << (int)header.Encoding << std::endl;
  
  // TODO: more buffer checks, palette handling
  
  {
    uint8_t* scanline = (header.NPlanes > 1 ?
			 new uint8_t[header.BytesPerLine * header.NPlanes] :
			 image.getRawData());
    for (int y = 0; y < image.h; ++y)
      {
	//std::cerr << "y: " << y << std::endl;
	for (int i = 0; i < header.BytesPerLine * header.NPlanes;)
	  {
	    uint8_t n = 1, v = stream->get();
	    if ((header.Encoding == 1) && (v & 0xC0) == 0xC0) {
	      n = v ^ 0xC0;
	      v = stream->get();
	    }
	    
	    //std::cerr << (int)n << "*: " << std::hex << (int)v << std::endl;
	    
	    while (n-- > 0 && i < header.BytesPerLine * header.NPlanes)
	      {
		//std::cerr << "i: " << i << std::endl;
		scanline[i++] = v;
	      }
	  }
	
	if (header.NPlanes > 1) // re-pack planes
	  {
	    for (int p = 0; p < header.NPlanes; ++p)
	      {
		uint8_t* dst = image.getRawData() + image.stride() * y;
		uint8_t* src = scanline + p * header.BytesPerLine;;
		for (int i = 0, j = p; i < header.BytesPerLine; ++i, j += header.NPlanes) {
		  dst[j] = src[i];
		}
	      }
	  }
	else // in-memory write
	  {
	    scanline += image.stride();
	  }
      }
    if (header.NPlanes > 1)
      delete[] scanline;
  }
  
  return true;
}

bool PCXCodec::writeImage(std::ostream* stream, Image& image, int quality,
			  const std::string& compress)
{
  PCXHeader header;
  
  header.Manufacturer = 10;
  header.Version = 5; // TODO: use lower version for non RGB images
  header.HDpi = image.xres;
  header.VDpi = image.yres;
  header.WindowXmin = 0;
  header.WindowYmin = 0;
  header.WindowXmax = image.width() - 1;
  header.WindowYmax = image.height() - 1;
  
  stream->write((char*)&header, sizeof(header));
  
  // TODO: write image data
  
  return false;
}

PCXCodec pcx_codec;
