/*
 * Floyd Steinberg dithering based on web publications.
 *
 * Copyright (C) 2006 - 2013 René Rebe, ExactCOD GmbH Germany
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

#include <math.h> // floor

#include "floyd-steinberg.h"
#include "ImageIterator2.hh"	

template <typename T>
struct FloydSteinberg_template
{
  void operator() (Image& image, int shades)
  {
    T it(image);
    typename T::accu a, a2;
    typename T::accu one = a.one();
    
    const int height = image.h, width = image.w;
    
    int direction = 1;
    
    const float factor = (float) (shades - 1) / (float) one.v[0];
    
    // row/error buffers
    float _error [width * image.spp];
    float* error = _error;
    float _nexterror [width  * image.spp];
    float* nexterror = _nexterror;
    
    // initialize the error buffers
    for (int x = 0; x < width * image.spp; ++x)
      error[x] = nexterror[x] = 0;
    
    for (int y = 0; y < height; ++y)
    {
      int start, end;
      
      for (int x = 0; x < width * image.spp; ++x)
	nexterror[x] = 0;
      
      if (direction == 1) {
	start = 0;
	end = width;
      }
      else {
	direction = -1;
	start = width - 1;
	end = -1;
      }
      
      it.at(start, y);      
      for (int x = start; x != end; x += direction)
      {
	a = *it;
	for (int channel = 0; channel < image.spp; ++channel)
	{
	  float newval = a.v[channel] + error[x * image.spp + channel];
	  
	  newval = floor (newval * factor + 0.5) / factor;
	  if (newval > one.v[0])
	    newval = one.v[0];
	  else if (newval < 0)
	    newval = 0;
	  
	  a2.v[channel] = newval + 0.5;
	  
	  float cerror = a.v[channel] + error[x * image.spp + channel] - a2.v[channel];
	  	  
	  // limit color bleeding, limit to /4 of the sample range
	  // TODO: make optional
	  if (fabs(cerror) > one.v[0] / 4) {
	    if (cerror < 0)
	      cerror = -one.v[0] / 4;
	    else
	      cerror = one.v[0] / 4;
	  }
	  
	  nexterror[x * image.spp + channel] += cerror * 5 / 16;
	  if (x + direction >= 0 && x + direction < width)
	  {
	    error[(x + direction) * image.spp + channel] += cerror * 7 / 16;
	    nexterror[(x + direction) * image.spp + channel] += cerror * 1 / 16;
	  }
	  if (x - direction >= 0 && x - direction < width)
	    nexterror[(x - direction) * image.spp + channel] += cerror * 3 / 16;
	  
	}
	it.set(a2);
	if (direction > 0)
	  ++it;
	else
	  --it;

      }
      
      // next row in the opposite direction
      direction *= -1;
      
      // swap error/nexterror
      float* tmp = error;
      error = nexterror;
      nexterror = tmp;
    }
  }
};

void FloydSteinberg (Image& image, int shades)
{
    codegen<FloydSteinberg_template> (image, shades);
}
