/*
 * Copyright (C) 2006 - 2008 René Rebe, ExactCODE GmbH Germany.
 *           (C) 2006, 2007 Archivista GmbH, CH-8042 Zuerich
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

#include <math.h>

#include <iostream>
#include <iomanip>

#include "ArgumentList.hh"

#include "Image.hh"
#include "ImageIterator2.hh"
#include "Codecs.hh"

#include "rotate.hh"

using namespace Utility;

void flipX (Image& image)
{
  // thru the codec?
  if (!image.isModified() && image.getCodec())
    if (image.getCodec()->flipX(image))
      return;
  
  const int stride = image.stride();
  uint8_t* data = image.getRawData();
  switch (image.spp * image.bps)
    {
    case 1:
    case 2:
    case 4:
      {
	// create a reversed bit table for fast lookup
	uint8_t reversed_bits[256];
	
	const int bps = image.bps;
	const int mask = (1 << bps) - 1;
	
	for (int i = 0; i < 256; ++i) {
	  uint8_t rev = 0, v = i;
	  for (int j = 0; j < 8/bps; ++j) {
	    rev = (rev << bps) | (v & mask);
	    v >>= bps;
	  }
	  reversed_bits[i] = rev;
	}
	
	for (int y = 0; y < image.h; ++y)
	  {
	    uint8_t* row = &data [y*stride];
	    for (int x = 0; x < stride/2; ++x) {
	      uint8_t v = row [x];
	      row[x] = reversed_bits [row[stride - 1 - x]];
	      row[stride - 1 - x] = reversed_bits[v];
	    }
	  }
      }
      break;
    case 8:
      {
	for (int y = 0; y < image.h; ++y)
	  {
	    uint8_t* row = &data [y*stride];
	    for (int x = 0; x < image.w/2; ++x) {
	      uint8_t v = row [x];
	      row[x] = row[image.w - 1 - x];
	      row[image.w - 1 - x] = v;
	    }
	  }
      }
      break;
    case 16:
      {
	for (int y = 0; y < image.h; ++y)
	  {
	    uint16_t* row = (uint16_t*) &data [y*stride];
	    for (int x = 0; x < image.w/2; ++x) {
	      uint16_t v = row [x];
	      row[x] = row[image.w - 1 - x];
	      row[image.w - 1 - x] = v;
	    }
	  }
      }
      break;
    case 24:
      {
	for (int y = 0; y < image.h; ++y) {
	  rgb* rgb_row = (rgb*) &data[y*stride]; 
	  for (int x = 0; x < image.w/2; ++x) {
	    rgb v = rgb_row [x];
	    rgb_row[x] = rgb_row[image.w - 1 - x];
	    rgb_row[image.w - 1 - x] = v;
	  }
	}
      }
      break;
    case 48:
      {
	for (int y = 0; y < image.h; ++y) {
	  rgb16* rgb_row = (rgb16*) &data[y*stride]; 
	  for (int x = 0; x < image.w/2; ++x) {
	    rgb16 v = rgb_row [x];
	    rgb_row[x] = rgb_row[image.w - 1 - x];
	    rgb_row[image.w - 1 - x] = v;
	  }
	}
      }
      break;
    default:
      std::cerr << "flipX: unsupported depth." << std::endl;
      return;
    }
  image.setRawData();
}

void flipY (Image& image)
{
  // thru the codec?
  if (!image.isModified() && image.getCodec())
    if (image.getCodec()->flipY(image))
      return;
  
  int bytes = image.stride();
  uint8_t* data = image.getRawData();
  for (int y = 0; y < image.h / 2; ++y)
    {
      int y2 = image.h - y - 1;

      uint8_t* row1 = &data[y*bytes];
      uint8_t* row2 = &data[y2*bytes];

      for (int x = 0; x < bytes; ++x)
	{
	  uint8_t v = *row1;
	  *row1++ = *row2;
	  *row2++ = v;
	}
    }
  image.setRawData();
}

void shear (Image& image, double xangle, double yangle)
{
  if (xangle != 0.0)
    {
    }
  if (yangle != 0.0)
    {
    }
  image.setRawData();
}

void rot90 (Image& image, int angle)
{
  bool cw = false; // clock-wise
  if (angle == 90)
    cw = true; // else 270 or -90 or whatever and thus counter cw
    
  int rot_bytes = (image.h*image.spp*image.bps + 7) / 8;
  
  uint8_t* data = image.getRawData();
  uint8_t* rot_data = (uint8_t*) malloc (rot_bytes * image.w);
  
  // TODO: 16bps
  switch (image.spp * image.bps)
    {
    case 1:
    case 2:
    case 4: {
      const int bps = image.bps;
      const int spb = 8 / bps; // Samples Per Byte
      const uint8_t mask =  0xF00 >> bps;
      // std::cerr << "mask: " << (int)mask << std::endl;
      
      for (int y = 0; y < image.h; ++y) {
	uint8_t* new_row;
	if (cw)
	  new_row = &rot_data [ (image.h - 1 - y) / spb ];
	else
	  new_row = &rot_data [ (image.w - 1) * rot_bytes + y / spb ];
	
	for (int x = 0; x < image.w;) {
	  // spread the bits thru the various row slots
	  uint8_t bits = *data++;
	  int i = 0;
	  for (; i < spb && x < image.w; ++i) {
	    if (cw) {
	      *new_row = *new_row >> bps | (bits & mask);
	      new_row += rot_bytes;
	    }
	    else {
	      *new_row = *new_row << bps | (bits & mask) >> (8-bps);
	      new_row -= rot_bytes;
	    }
	    bits <<= bps;
	    ++x;
	  }
	  // finally shift the last line if necessary
	  // TODO: recheck this residual bit for correctness
	  if (i < spb) {
	    if (cw) {
	      new_row -= rot_bytes;
	      *new_row = *new_row >> (8 - (bps*i));
	    }
	    else {
	      new_row += rot_bytes;
	      *new_row = *new_row << (8 - (bps*i));
	      
	    }
	    bits <<= 1;
	    ++x;
	  }
	}
      }
    }
      break;
      
    case 8:
      for (int y = 0; y < image.h; ++y) {
	uint8_t* new_row;
	if (cw)
	  new_row = &rot_data [ image.h - 1 - y ];
	else
	  new_row = &rot_data [ (image.w - 1) * rot_bytes + y ];
	for (int x = 0; x < image.w; ++x) {
	  *new_row = *data++;
	  if (cw)
	    new_row += rot_bytes;
	  else
	    new_row -= rot_bytes;
	}
      }
      break;
      
    case 24:
      {
	rgb* rgb_data = (rgb*) image.getRawData(); 
	for (int y = 0; y < image.h; ++y) {
	  rgb* new_row;
	  if (cw)
	    new_row = (rgb*)
	      &rot_data [ (image.h - 1 - y) * image.spp ];
	  else
	    new_row = (rgb*)
	      &rot_data [ (image.w - 1) * rot_bytes + (y * image.spp) ];
	  for (int x = 0; x < image.w; ++x) {
	    *new_row = *rgb_data++;
	    if (cw)
	      new_row += image.h;
	    else
	      new_row -= image.h;
	  }
	}
      }
      break;
      
    default:
      std::cerr << "rot90: unsupported depth. spp: " << image.spp << ", bpp:" << image.bps << std::endl;
      free (rot_data);
      return;
    }
  
  // we are done, tweak the w/h
  int x = image.w;
  image.w = image.h;
  image.h = x;
  // resolution, likewise
  x = image.xres;
  image.xres = image.yres;
  image.yres = x;

  // set the new data
  image.setRawData (rot_data);
}

template <typename T>
struct rotate_template
{
  void operator() (Image& image, double angle, const Image::iterator& background)
  {
    angle = fmod (angle, 360);
    if (angle < 0)
      angle += 360;
  
    if (angle == 0.0)
      return;
  
    // trivial code just for testing, to be optimized
  
    angle = angle / 180 * M_PI;
  
    const int xcent = image.w / 2;
    const int ycent = image.h / 2;
  
    Image orig_image; orig_image.copyTransferOwnership(image);
    image.resize (image.w, image.h);

    const double cached_sin = sin (angle);
    const double cached_cos = cos (angle);
  
    std::cerr << "angle: " << angle << std::endl;
    
    T it (image);
    T orig_it (orig_image);
  
    for (int y = 0; y < image.h; ++y)
      for (int x = 0; x < image.w; ++x)
	{
	  double ox =   (x - xcent) * cached_cos + (y - ycent) * cached_sin;
	  double oy = - (x - xcent) * cached_sin + (y - ycent) * cached_cos;
	  
	  ox += xcent;
	  oy += ycent;
	  
	  typename T::accu a;
	  
	  if (ox >= 0 && oy >= 0 &&
	      ox < image.w && oy < image.h)
	    {
	      int oxx = (int)floor(ox);
	      int oyy = (int)floor(oy);
	      
	      int oxx2 = std::min (oxx + 1, image.w - 1);
	      int oyy2 = std::min (oyy + 1, image.h - 1);
	      
	      int xdist = (int) ((ox - oxx) * 256);
	      int ydist = (int) ((oy - oyy) * 256);
	      
	      a  = (*orig_it.at(oxx,  oyy))  * ((256 - xdist) * (256 - ydist));
	      a += (*orig_it.at(oxx2, oyy))  * (xdist         * (256 - ydist));
	      a += (*orig_it.at(oxx,  oyy2)) * ((256 - xdist) * ydist);
	      a += (*orig_it.at(oxx2, oyy2)) * (xdist         * ydist);
	      a /= (256 * 256);
	    }
	  else
	    a = (background);
	  
	  it.set (a);
	  ++it;
	}
    image.setRawData ();
  }
};

void rotate (Image& image, double angle, const Image::iterator& background)
{
  codegen<rotate_template> (image, angle, background);
}

template <typename T>
struct copy_crop_rotate_template
{
  Image* operator() (Image& image, int x_start, int y_start,
		     unsigned int w, unsigned int h,
		     double angle, const Image::iterator& background)
  {
    angle = fmod (angle, 360);
    if (angle < 0)
      angle += 360;
    
    // trivial code just for testing, to be optimized
    
    angle = angle / 180 * M_PI;
    
    Image* new_image = new Image;
    new_image->copyMeta (image);
    new_image->resize (w, h);
    
    T it (*new_image);
    T orig_it (image);
    
    const double cached_sin = sin (angle);
    const double cached_cos = cos (angle);
    
    for (unsigned int y = 0; y < h; ++y)
      for (unsigned int x = 0; x < w; ++x)
	{
	  const double ox = ( (double)x * cached_cos + (double)y * cached_sin) + x_start;
	  const double oy = (-(double)x * cached_sin + (double)y * cached_cos) + y_start;
	  
	  typename T::accu a;
	  
	  if (ox >= 0 && oy >= 0 &&
	      ox < image.w && oy < image.h) {
	    
	    int oxx = (int)floor(ox);
	    int oyy = (int)floor(oy);
	    
	    int oxx2 = std::min (oxx+1, image.w-1);
	    int oyy2 = std::min (oyy+1, image.h-1);
	    
	    int xdist = (int) ((ox - oxx) * 256);
	    int ydist = (int) ((oy - oyy) * 256);
	    
	    a  = (*orig_it.at(oxx,  oyy))  * ((256 - xdist) * (256 - ydist));
	    a += (*orig_it.at(oxx2, oyy))  * (xdist         * (256 - ydist));
	    a += (*orig_it.at(oxx,  oyy2)) * ((256 - xdist) * ydist);
	    a += (*orig_it.at(oxx2, oyy2)) * (xdist         * ydist);
	    a /= (256 * 256);
	  }
	  else
	    a = (background);
	  
	  it.set (a);
	  ++it;
	}
    return new_image;
  }
};

Image* copy_crop_rotate (Image& image, int x_start, int y_start,
			 unsigned int w, unsigned int h,
			 double angle, const Image::iterator& background)
{
  return codegen_return<Image*, copy_crop_rotate_template> (image, x_start, y_start,
							    w, h, angle, background);
}
