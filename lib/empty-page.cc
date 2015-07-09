/*
 * Copyright (C) 2005 - 2015 René Rebe
 *           (C) 2005 - 2007 Archivista GmbH, CH-8042 Zuerich
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

#include <iostream>

#include "Bits.hh"
#include "Image.hh"

#include "Colorspace.hh"
#include "Matrix.hh"

#include "empty-page.hh"
#include "optimize2bw.hh"

/* TODO: for more accurance one could introduce a hot-spot area that
   has a higher weight than the other (outer) region to more reliably
   detect crossed but otherwise empty pages */
bool detect_empty_page (Image& im, double percent, int marginH, int marginV,
			int* set_pixels)
{
  // sanitize margins
  if (marginH % 8 != 0)
    marginH -= marginH % 8;
  
  Image* image, img;
  
  if (im.spp == 1 && im.bps == 1) {
    image = &im;
  }
  // already in sub-byte domain? just count the black pixels
  else if (im.spp == 1 && im.bps < 8) {
    img = im; image = &img;
    colorspace_by_name (*image, "gray1");
  }
  // if not 1-bit, yet: convert it down ...
  else {
    img = im; image = &img;
    // don't care about cmyk vs. rgb, just get gray8 pixels, quickly
    colorspace_by_name (*image, "gray8");

    // force quick pass, no color use, 1px radius
    optimize2bw (*image, 0/*min*/, 0/*max*/, 128/*thr*/, 0/*sloppy*/, 1/*radius*/);
    // convert to 1-bit (threshold) - optimize2bw does not perform that step ...
    colorspace_gray8_to_gray1 (*image);
  }
  
  const int stride = image->stride();
  
  // count pixels by table lookup
  int pixels = 0;
  uint8_t* data = image->getRawData();
  for (int row = marginV; row < image->h - marginV; ++row)
  {
    uint8_t* rowptr = data + stride * row;
    for (int x = marginH/8; x < stride - marginH/8; ++x) {
      int b = Exact::popcount[rowptr[x]];
      // it is a bitsset table - and we want the zeros ...
      pixels += 8-b;
    }
  }

  float image_percent = 100.0 * pixels / (image->w * image->h);
  
  if (set_pixels)
    *set_pixels = pixels;
  
  return image_percent < percent;
}
