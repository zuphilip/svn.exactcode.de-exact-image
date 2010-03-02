/*
 * Copyright (C) 2009 René Rebe, ExactCODE GmbH Germany.
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
 */

#include "canvas.hh"
#include "Colorspace.hh"

#include <string.h>

void append (Image& image, Image& other)
{
  // TODO: currently we support images with the same with
  if (image.w != other.w) {
    std::cerr << "image append: different image width unimplemented" << std::endl;
    return;
  }
  
  // must be in the same colorspace
  colorspace_by_name(other, colorspace_name(image));
  
  // resize height
  const unsigned int old_height = image.h;
  image.resize(image.w, image.h + other.h);
  
  // copy raw content
  memcpy(image.getRawData() + image.stride() * old_height,
	 other.getRawData(),
	 other.stride() * other.h);
}

void copy (Image& image, int x,	int y, int w, int h, Image& other, int sx, int sy)
{
  //const int rx = sx - x;
  //const int ry = sy - y;

  // TODO: clip

  // must be in the same colorspace
  colorspace_by_name(other, colorspace_name(image));
  
  const int bpp = (image.bps * image.spp + 7) / 8;
  uint8_t* dst = image.getRawData() + image.stride() * y + bpp * x;
  uint8_t* src = other.getRawData() + other.stride() * sy + bpp *sx;
  
  if (y <= sy) {
    for (; h > 0; --h, dst += image.stride(), src += other.stride())
      memmove(dst, src, bpp * w);
  } else {
    dst += image.stride() * (h-1);
    src += other.stride() * (h-1);
    
    for (; h > 0; --h, dst -= image.stride(), src -= other.stride())
      memmove(dst, src, bpp * w);
  }
}
