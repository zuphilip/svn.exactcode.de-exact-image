/*
 * Copyright (C) 2006 René Rebe
 *           (C) 2006 Archivista GmbH, CH-8042 Zuerich
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

#include <math.h>

#include <iostream>
#include <iomanip>

#include "ArgumentList.hh"

#include "tiff.hh"
#include "Image.hh"

using namespace Utility;

void flipX (Image& image)
{
  int bytes = image.Stride();
  
  switch (image.spp * image.bps)
    {
    case 1:
      {
	// create a reversed bit table for fast lookup
	int reversed_bits[256];
	for (int i = 0; i < 256; ++i) {
	  char rev = 0, v = i;
	  for (int j = 0; j < 8; ++j) {
	    rev = rev << 1 | v & 1;
	    v >>= 1;
	  }
	  reversed_bits[i] = rev;
	}
	
	for (int y = 0; y < image.h; ++y)
	  {
	    unsigned char* row = &image.data [y*bytes];
	    for (int x = 0; x < bytes/2; ++x) {
	      unsigned char v = row [x];
	      row[x] = reversed_bits [row[bytes - 1 - x]];
	      row[bytes - 1 - x] = reversed_bits[v];
	    }
	  }
      }
      break;
    case 8:
      {
	for (int y = 0; y < image.h; ++y)
	  {
	    unsigned char* row = &image.data [y*bytes];
	    for (int x = 0; x < bytes/2; ++x) {
	      unsigned char v = row [x];
	      row[x] = row[bytes - 1 - x];
	      row[bytes - 1 - x] = v;
	    }
	  }
      }
      break;
    case 24:
      {
	for (int y = 0; y < image.h; ++y) {
	  rgb* rgb_row = (rgb*) &image.data[y*bytes]; 
	  for (int x = 0; x < image.w/2; ++x) {
	    rgb v = rgb_row [x];
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
}

void flipY (Image& image)
{
  // TODO: 16bps
  int bytes = image.Stride();
  for (int y = 0; y < image.h / 2; ++y)
    {
      int y2 = image.h - y - 1;

      unsigned char* row1 = &image.data[y*bytes];
      unsigned char* row2 = &image.data[y2*bytes];

      for (int x = 0; x < bytes; ++x)
	{
	  unsigned char v = *row1;
	  *row1++ = *row2;
	  *row2++ = v;
	}
    }
}

void shear (Image& image, double xangle, double yangle)
{
  if (xangle != 0.0)
    {
    }
  if (yangle != 0.0)
    {
    }
}

void rot90 (Image& image, int angle)
{
  bool cw = false; // clock-wise
  if (angle == 90)
    cw = true; // else 270 or -90 or whatever and thus counter cw
    
  int rot_bytes = (image.h*image.spp*image.bps + 7) / 8;
  
  unsigned char* data = image.data;
  unsigned char* rot_data = (unsigned char*) malloc (rot_bytes * image.w);

  switch (image.spp * image.bps)
    {
    case 1:
      for (int y = 0; y < image.h; ++y) {
	unsigned char* new_row;
	if (cw)
	  new_row = &rot_data [ (image.h - 1 - y) / 8 ];
	else
	  new_row = &rot_data [ (image.w - 1) * rot_bytes + y / 8 ];
	for (int x = 0; x < image.w;) {
	  // spread the bits thru the various row slots
	  unsigned char bits = *data++;
	  int i = 0;
	  for (; i < 8 && x < image.w; ++i) {
	    if (cw) {
	      *new_row = *new_row >> 1 | (bits & 0x80);
	      new_row += rot_bytes;
	    }
	    else {
	      *new_row = *new_row << 1 | (bits & 0x80) >> 7;
	      new_row -= rot_bytes;
	    }
	    bits <<= 1;
	    ++x;
	  }
	  // finally shift the last line if necessary
	  if (i < 8) {
	    if (cw) {
	      new_row -= rot_bytes;
	      *new_row = *new_row >> (8 - i);
	    }
	    else {
	      new_row += rot_bytes;
	      *new_row = *new_row << (8 - i);
	      
	    }
	    bits <<= 1;
	    ++x;
	  }
	}
      }
      break;
      
    case 8:
      for (int y = 0; y < image.h; ++y) {
	unsigned char* new_row;
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
	rgb* rgb_data = (rgb*) image.data; 
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
  
  // set the new data
  free (image.data);
  image.data = rot_data;
  
  // we are done, tweak the w/h
  int x = image.w;
  image.w = image.h;
  image.h = x;
  // resolution, likewise
  x = image.xres;
  image.xres = image.yres;
  image.yres = x;
}


void rotate (Image& image, double angle, Image::iterator background)
{
  angle = fmod (angle, 360);
  if (angle < 0)
    angle += 360;
  
  if (angle == 0.0)
    return;
  
  if (angle == 180.0) {
    flipX (image);
    flipY (image);
    return;
  }

  if (angle == 90.0) {
    rot90 (image, 90);
    return;
  }
 
  if (angle == 270.0) {
    rot90 (image, 270);
    return;
  }

  // trivial code just for testing, to be optimized
  
  angle = angle / 180 * M_PI;
  
  const int xcent = image.w/2;
  const int ycent = image.h/2;
  
  Image orig_image = image;
  
  image.data = (uint8_t*) malloc (image.Stride()*image.h);
  Image::iterator it = image.begin();
  Image::iterator orig_it = orig_image.begin();
  
  double cached_sin = sin (angle);;
  double cached_cos = cos (angle);

  for(int y = 0; y < image.h; ++y)
      for(int x = 0; x < image.w; ++x)
	{
	  double ox =   (x-xcent) * cached_cos + (y-ycent) * cached_sin;
	  double oy = - (x-xcent) * cached_sin + (y-ycent) * cached_cos;
	  
	  ox += xcent;
	  oy += ycent;
	  
	  if (ox >= 0 && oy >= 0 &&
	      ox < image.w && oy < image.h) {
	    
	    int oxx = (int)floor(ox);
	    int oyy = (int)floor(oy);
	    
	    int oxx2 = std::min (oxx+1, image.w);
	    int oyy2 = std::min (oyy+1, image.h);
	    
	    int xdist = (int) ((ox - oxx) * 256);
	    int ydist = (int) ((oy - oyy) * 256);
	    
	    it.set ( (
		      *orig_it.at (oxx,  oyy ) * (256-xdist) * (256-ydist) +
		      *orig_it.at (oxx2, oyy ) * xdist       * (256-ydist) +
		      *orig_it.at (oxx,  oyy2) * (256-xdist) * ydist +
		      *orig_it.at (oxx2, oyy2) * xdist       * ydist
		      ) /
		     (256 * 256) );
	  }
	  else
	    it.set (background);
	  
	  ++it;
	}
}
