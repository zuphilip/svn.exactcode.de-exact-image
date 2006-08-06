
/*
 * Copyright (C) 2006 René Rebe
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

#include <ImfRgbaFile.h>
#include <ImfArray.h>

#include <algorithm>

#include "openexr.hh"

#include "Colorspace.hh"

using namespace Imf;
using namespace Imath;

using std::cout;
using std::endl;

bool OpenEXRLoader::readImage (FILE* file, Image& image)
{
  RgbaInputFile exrfile ("testsuite/openexr/GoldenGate.exr");
  Box2i dw = exrfile.dataWindow ();
  
  image.w = dw.max.x - dw.min.x + 1;
  image.h = dw.max.y - dw.min.y + 1;
  Array2D<Rgba> pixels (1, image.w); // working data
  
  while (dw.min.y <= dw.max.y)
    {
      cout << "> " << dw.min.y << endl;
      exrfile.setFrameBuffer (&pixels[0][0] - dw.min.x - dw.min.y * image.w,
			      1, image.w);
      exrfile.readPixels (dw.min.y, std::min (dw.min.y + 1, dw. max.y));
      // process pixels
      dw.min.y += 1;
    }
  
  return true;
}

bool OpenEXRLoader::writeImage (FILE* file, Image& image,
				int quality, const std::string& compress)
{
  RgbaChannels type = WRITE_RGBA;
  Box2i displayWindow (V2i (0, 0), V2i (image.w - 1, image.h - 1));
  RgbaOutputFile exrfile ("testsuite/openexr/GoldenGate.exr",
			  image.w, image.h, type);
  
  Array2D<Rgba> pixels (1, image.w); // working data
  
  for (int y = 0; y < image.h; ++y)
    {
      
      exrfile.setFrameBuffer (&pixels[0][0] - y * image.w,
			      1, image.w);
      exrfile.writePixels (1);
    }
  return true;
}

OpenEXRLoader openexr_loader;
