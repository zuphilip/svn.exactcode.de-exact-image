
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

#include <math.h>

#include <iostream>
#include <iomanip>

#include "ArgumentList.hh"

#include "tiff.hh"
#include "jpeg.hh"

using namespace Utility;

class Image
{
public:

  Image ()
    : data(0) {
  }
  
  ~Image () {
    if (data)
      free (data);
  }

  
  int w, h, bps, spp, xres, yres;
  unsigned char* data;
};


void flipX (Image& image)
{
  int bytes = (image.w*image.spp*image.bps + 7) / 8;
  
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
	typedef struct { unsigned char r, g, b; } rgb;
	
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
      std::cerr << "flipX: unsuported depth." << std::endl;
      return;
    }
}

void flipY (Image& image)
{
  int bytes = (image.w*image.spp*image.bps + 7) / 8;
  for (int y = 0; y < image.h / 2; ++y)
    {
      int y2 = image.h - y;

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

int main (int argc, char* argv[])
{
  ArgumentList arglist;
  
  // setup the argument list
  Argument<bool> arg_help ("", "help",
			   "display this help text and exit");
  Argument<std::string> arg_input ("i", "input", "input file",
                                   1, 1);
  Argument<std::string> arg_output ("o", "output", "output file",
				    1, 1);
  
  Argument<int> arg_angle ("r", "angle",
			   "rotation angle", 0, 1, 1);

  arglist.Add (&arg_help);
  arglist.Add (&arg_input);
  arglist.Add (&arg_output);
  arglist.Add (&arg_angle);

  // parse the specified argument list - and maybe output the Usage
  if (!arglist.Read (argc, argv) || arg_help.Get() == true)
    {
      std::cerr << "Fast rotator (especially for orthogonal angles)"
                <<  " - Copyright 2006 by René Rebe" << std::endl
                << "Usage:" << std::endl;
      
      arglist.Usage (std::cerr);
      return 1;
    }
  
  Image image;
  image.data = read_TIFF_file (arg_input.Get().c_str(),
			       &image.w, &image.h,
			       &image.bps, &image.spp,
			       &image.xres, &image.yres);
  if (!image.data)
  {
    std::cerr << "Error reading JPEG." << std::endl;
    return 1;
  }
  
  std::cout << "spp: " << image.spp << ", bps: " << image.bps << std::endl;
  
  // rotation algorithms
  switch (arg_angle.Get())
    {
    case 0:
      ; // NOP
    case 180: 
      // IM: user	0m1.324s
      {
	flipX (image);
	flipY (image);
	
      }
      break;
    default:
      std::cerr << "Rotation angle not yet supported." << std::endl;
    }
  
  
  write_TIFF_file (arg_output.Get().c_str(), image.data, image.w, image.h,
		   image.bps, image.spp, image.xres, image.yres);
  
  return 0;
}
