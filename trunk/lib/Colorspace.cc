/*
 * Colorspace conversions..
 * Copyright (C) 2006, 2007 René Rebe, ExactCODE
 * Copyright (C) 2007 Susanne Klaus, ExactCODE
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

#include <iostream>

#include "Image.hh"
#include "Codecs.hh"
#include "Colorspace.hh"

#include "Endianess.hh"

void normalize_gray8 (Image& image, uint8_t low, uint8_t high)
{
  int histogram[256] = { 0 };
  
  for (uint8_t* it = image.getRawData(); it < image.getRawDataEnd();)
    histogram[*it++]++;

  int lowest = 255, highest = 0;
  for (int i = 0; i <= 255; i++)
    {
      // std::cout << i << ": "<< histogram[i] << std::endl;
      if (histogram[i] > 2) // magic denoise constant
	{
	  if (i < lowest)
	    lowest = i;
	  if (i > highest)
	    highest = i;
	}
    }
  std::cerr << "lowest: " << lowest << ", highest: " << highest << std::endl;
  
  if (low)
    lowest = low;
  if (high)
    highest = high;
    
  // TODO use options
  signed int a = (255 * 256) / (highest - lowest);
  signed int b = -a * lowest;

  std::cerr << "a: " << (float) a / 256
	    << " b: " << (float) b / 256 << std::endl;

  for (uint8_t* it = image.getRawData(); it < image.getRawDataEnd(); ++it)
    *it = ((int) *it * a + b) / 256;

  image.setRawData();
}

void normalize (Image& image, uint8_t l, uint8_t h)
{
  if (image.bps == 8 && image.spp == 1)
    return normalize_gray8 (image, l, h);
  
  double low = 1.0;
  double high = 0.0;

  Image::iterator it = image.begin();
  
  double r, g, b;
  for (; it != image.end(); ++it) {
    *it;
    it.getRGB(r, g, b);

    if (r < low)
      low = r;
    if (r > high)
      high = r;

    if (g < low)
      low = g;
    if (g > high)
      high = g;
    
    if (b < low)
      low = b;
    if (b > high)
      high = b;
  }
  
  const double fa = 1.0 / (high - low);
  const double fb = fa * low;
 
  std::cerr << "low: " << low << ", high: " << high << std::endl;
  std::cerr << "a: " << fa<< " b: " << fb << std::endl;
  
  for (it = image.begin(); it != image.end(); ++it) {
    *it;
    it.getRGB (r, g, b);
    it.setRGB (r * fa - fb, g * fa - fb, b * fa - fb);
    it.set (it);
  }
}

void colorspace_rgba8_to_rgb8 (Image& image)
{
  uint8_t* output = image.getRawData();
  for (uint8_t* it = image.getRawData(); it < image.getRawData() + image.w*image.h*image.spp;)
    {
      *output++ = *it++;
      *output++ = *it++;
      *output++ = *it++;
      it++; // skip over a
    }
  image.spp = 3; // converted data right now
  image.setRawData();
}

void colorspace_rgb8_to_gray8 (Image& image)
{
  uint8_t* output = image.getRawData();
  for (uint8_t* it = image.getRawData(); it < image.getRawData() + image.w*image.h*image.spp;)
    {
      // R G B order and associated weighting
      int c = (int)*it++ * 28;
      c += (int)*it++ * 59;
      c += (int)*it++ * 11;
      
      *output++ = (uint8_t)(c / 100);
    }
  image.spp = 1; // converted data right now
  image.setRawData();
}

void colorspace_gray8_threshold (Image& image, unsigned char threshold)
{
  uint8_t *output = image.getRawData();
  uint8_t *input = image.getRawData();
  
  for (int row = 0; row < image.h; row++)
    {
      for (int x = 0; x < image.w; x++)
	{
	  *output++ = *input++ > threshold ? 0xFF : 0x00;
	}
    }
  image.setRawData();
}

void colorspace_gray8_denoise_neighbours (Image &image)
{
  // we need some pixels to compare, also avoids conditionals
  // below
  if (image.w < 3 ||
      image.h < 3)
    return;
  
  uint8_t* it = image.getRawData();
  
  struct compare_and_set
  {
    const Image& image;
    const unsigned int stride;
    compare_and_set (const Image& _image)
      : image(_image), stride (image.stride())
    {
    }
    
    // without the inner(area) compiler guidance the conditionals are
    // not optimized away well enough
    void operator() (const int x, const int y, uint8_t* it,
		     const bool inner = false)
    {
      int n = 0;
      unsigned int sum = 0;
      
      if (inner || x > 0)
	sum += *(it-1), ++n;
      if (inner || y > 0)
	sum += *(it-stride), ++n;
      if (inner || x < image.w-1)
	sum += *(it+1), ++n;
      if (inner || y < image.h-1)
	sum += *(it+stride), ++n;
      
      // if all direct neighbours are black or white, fill it
      if (sum == 0)
	*it = 0;
      else if (sum == n * 0xff)
	*it = 0xff;
    }
  } compare_and_set (image);
  
  for (int y = 0; y < image.h; ++y)
    {
      // optimize conditionals away for the inner area
      if (y > 0 && y < image.h-1)
	{
	  compare_and_set (1, y, it++);
	  for (int x = 1; x < image.w-1; ++x, ++it)
	    compare_and_set (x, y, it, true);
	  compare_and_set (image.w-1, y, it++);
	}
      else // quite some out of bounds conditions to check
	for (int x = 0; x < image.w; ++x, ++it) 
	  compare_and_set (x, y, it);
    }
    
  image.setRawData();
}

void colorspace_gray8_to_gray1 (Image& image, uint8_t threshold)
{
  uint8_t *output = image.getRawData();
  uint8_t *input = image.getRawData();
  
  for (int row = 0; row < image.h; row++)
    {
      uint8_t z = 0;
      int x = 0;
      for (; x < image.w; x++)
	{
	  z <<= 1;
	  if (*input++ > threshold)
	    z |= 0x01;
	  
	  if (x % 8 == 7)
	    {
	      *output++ = z;
	      z = 0;
	    }
	}
      int remainder = 8 - x % 8;
      if (remainder != 8)
	{
	  z <<= remainder;
	  *output++ = z;
	}
    }
  image.bps = 1;
  image.setRawData();
}

void colorspace_gray8_to_gray4 (Image& image)
{
  uint8_t *output = image.getRawData();
  uint8_t *input = image.getRawData();
  
  for (int row = 0; row < image.h; row++)
    {
      uint8_t z = 0;
      int x = 0;
      for (; x < image.w; x++)
	{
	  z <<= 4;
	  z |= (*input++ >> 4) & 0xF;
	  
	  if (x % 2 == 1)
	    {
	      *output++ = z;
	      z = 0;
	    }
	}
      int remainder = 2 - x % 2;
      if (remainder != 2)
	{
	  z <<= 4*remainder;
	  *output++ = z;
	}
    }
  image.bps = 4;
  image.setRawData();
}
void colorspace_gray8_to_gray2 (Image& image)
{
  uint8_t *output = image.getRawData();
  uint8_t *input = image.getRawData();
  
  for (int row = 0; row < image.h; ++row)
    {
      uint8_t z = 0;
      int x = 0;
      for (; x < image.w; x++)
	{
	  z <<= 2;
	  z |= (*input++ >> 6) & 0x3;
	  
	  if (x % 4 == 3)
	    {
	      *output++ = z;
	      z = 0;
	    }
	}
      int remainder = 4 - x % 4;
      if (remainder != 4)
	{
	  z <<= 2*remainder;
	  *output++ = z;
	}
    }
  image.bps = 2;
  image.setRawData();
}

void colorspace_gray8_to_rgb8 (Image& image)
{
  uint8_t* data = (uint8_t*)malloc (image.w*image.h*3);
  uint8_t* output = data;
  for (uint8_t* it = image.getRawData ();
       it < image.getRawData() + image.w*image.h*image.spp; ++it)
    {
      *output++ = *it;
      *output++ = *it;
      *output++ = *it;
    }
  image.spp = 3; // converted data right now
  image.setRawData(data);
}

void colorspace_grayX_to_gray8 (Image& image)
{
  // sanity and for compiler optimization
  if (image.spp != 1)
    return;

  Image gray8_image;
  gray8_image.bps = 8;
  gray8_image.spp = 1;
  gray8_image.resize (image.w, image.h);

  Image::iterator it = image.begin();
  Image::iterator gray8_it = gray8_image.begin();

  while (gray8_it != gray8_image.end()) {
    *it;
    gray8_it.setL (it.getL());
    gray8_it.set (gray8_it);
    ++it; ++gray8_it;
  }
  image = gray8_image;
}

void colorspace_grayX_to_rgb8 (Image& image)
{
  // sanity and for compiler optimization
  if (image.spp != 1)
    return;
  
  Image rgb_image;
  rgb_image.bps = 8;
  rgb_image.spp = 3;
  rgb_image.resize (image.w, image.h);
  
  Image::iterator it = image.begin();
  Image::iterator rgb_it = rgb_image.begin();
  
  while (rgb_it != rgb_image.end()) {
    *it;
    rgb_it.setL (it.getL());
    rgb_it.set (rgb_it);
    ++it; ++rgb_it;
  }
  image = rgb_image;
}

void colorspace_gray1_to_gray2 (Image& image)
{
  uint8_t* old_data = image.getRawData();
  int old_stride = image.stride();
  
  image.bps = 2;
  image.setRawDataWithoutDelete ((uint8_t*) malloc (image.h*image.stride()));
  uint8_t* output = image.getRawData();
  
  for (int row = 0; row < image.h; ++row)
    {
      uint8_t z = 0;
      uint8_t zz = 0;
      uint8_t* input  = old_data + row * old_stride;

      int x;
      for (x = 0; x < image.w; ++x)
	{
	  if (x % 8 == 0)
	    z = *input++;

	  zz <<= 2;
	  if (z >> 7)
	    zz |= 0x3;
	  z <<= 1;
	  
	  if (x % 4 == 3)
	    *output++ = zz;
	}
      
      int remainder = 4 - x % 4;
      if (remainder != 4)
	{
	  zz <<= 2*remainder;
	  *output++ = zz;
	}
    }
  free (old_data);
}

void colorspace_gray1_to_gray4 (Image& image)
{
  uint8_t* old_data = image.getRawData();
  int old_stride = image.stride();
  
  image.bps = 4;
  image.setRawDataWithoutDelete ((uint8_t*) malloc (image.h*image.stride()));
  uint8_t* output = image.getRawData();
  
  for (int row = 0; row < image.h; ++row)
    {
      uint8_t z = 0;
      uint8_t zz = 0;
      
      uint8_t* input  = old_data + row * old_stride;
      
      int x;
      for (x = 0; x < image.w; ++x)
	{
	  if (x % 8 == 0)
	    z = *input++;
	  
	  zz <<= 4;
	  if (z >> 7)
	    zz |= 0x0F;
	  z <<= 1;
	  
	  if (x % 2 == 1)
	    *output++ = zz;
	}
      
      int remainder = 2 - x % 2;
      if (remainder != 2)
	{
	  zz <<= 4*remainder;
	  *output++ = zz;
	}
    }
  
  free (old_data);
}

void colorspace_gray1_to_gray8 (Image& image)
{
  uint8_t* old_data = image.getRawData();
  int old_stride = image.stride();
  
  image.bps = 8;
  image.setRawDataWithoutDelete ((uint8_t*) malloc (image.h*image.stride()));
  uint8_t* output = image.getRawData();
 
  for (int row = 0; row < image.h; ++row)
    {
      uint8_t* input  = old_data + row * old_stride;
      uint8_t z = 0;
      
      for (int x = 0; x < image.w; ++x)
	{
	  if (x % 8 == 0)
	    z = *input++;
	  
	  *output++ = (z >> 7) ? 0xff : 0x00;
	  
	  z <<= 1;
	}
    }
  free (old_data);
}

void colorspace_16_to_8 (Image& image)
{
  uint8_t* output = image.getRawData();
  for (uint8_t* it = image.getRawData();
       it < image.getRawDataEnd();)
    {
      if (Exact::NativeEndianTraits::IsBigendian)
	*output++ = it[0];
      else
	*output++ = it[1];
      it += 2;
    }
  image.bps = 8; // converted 8bit data
  image.setRawData ();
}

void colorspace_8_to_16 (Image& image)
{
  uint16_t* data = (uint16_t*) malloc (image.w*image.h*image.spp*2);
  uint16_t* out_it = data;
  for (uint8_t* it = image.getRawData();
       it < image.getRawDataEnd();)
    {
      *out_it++ = *it++ * 0xffff / 255;
    }
  
  image.bps = 16; // converted 16bit data
  image.setRawData ((uint8_t*)data);
}

void colorspace_de_palette (Image& image, int table_entries,
			    uint16_t* rmap, uint16_t* gmap, uint16_t* bmap)
{
  // detect 1bps b/w tables
  if (image.bps == 1) {
    if (rmap[0] == 0 &&
	gmap[0] == 0 &&
	bmap[0] == 0 &&
	rmap[1] >= 0xff00 &&
	gmap[1] >= 0xff00 &&
	bmap[1] >= 0xff00)
      {
	//std::cerr << "correct b/w table." << std::endl;
	return;
      }
    if (rmap[1] == 0 &&
	gmap[1] == 0 &&
	bmap[1] == 0 &&
	rmap[0] >= 0xff00 &&
	gmap[0] >= 0xff00 &&
	bmap[0] >= 0xff00)
      {
	//std::cerr << "inverted b/w table." << std::endl;
	for (uint8_t* it = image.getRawData();
	     it < image.getRawDataEnd();
	     ++it)
	  *it ^= 0xff;
	image.setRawData ();
	return;
      }
  }
  
  // detect gray tables
  bool is_gray = false;
  if (table_entries > 1) {
    int i;
    // std::cerr << "checking for gray table" << std::endl;
    for (i = 0; i < table_entries; ++i) {
      int ref = i * 255 / (table_entries-1);
      if (rmap[i] >> 8 != ref || rmap[i] >> 8 != gmap[i] >> 8 || rmap[i] >> 8 != bmap[i] >> 8)
	break;
    }
    
    if (i == table_entries) {
      // std::cerr << "found gray table." << std::endl;
      is_gray = true;
    }
    
    // shortpath if the data is already 8bit gray
    if (is_gray && (image.bps == 8 || image.bps == 4 || image.bps == 2) ) {
      // std::cerr << "is gray table" << std::endl;
      return;
    }
  }
  
  int new_size = image.w * image.h;
  if (!is_gray) // RGB
    new_size *= 3;
  
  uint8_t* orig_data = image.getRawData();
  uint8_t* new_data = (uint8_t*) malloc (new_size);
  
  uint8_t* src = orig_data;
  uint8_t* dst = new_data;

  // TODO: allow 16bit output if the palette contains that much dynamic

  int bits_used = 0;
  int x = 0;
  while (dst < new_data + new_size)
    {
      uint8_t v = *src >> (8 - image.bps);
      if (is_gray) {
	*dst++ = rmap[v] >> 8;
      } else {
	*dst++ = rmap[v] >> 8;
	*dst++ = gmap[v] >> 8;
	*dst++ = bmap[v] >> 8;
      }
      
      bits_used += image.bps;
      ++x;

      if (bits_used == 8 || x == image.w) {
	++src;
	bits_used = 0;
	if (x == image.w)
	  x = 0;
      }
      else {
	*src <<= image.bps;
      }
    }
  image.bps = 8;
  if (is_gray)
    image.spp = 1;
  else
    image.spp = 3;

  image.setRawData (new_data);  
}

bool colorspace_by_name (Image& image, const std::string& target_colorspace)
{
  std::string space = target_colorspace;
  std::transform (space.begin(), space.end(), space.begin(), tolower);
  
  // thru the codec?
  if (!image.isModified() && image.getCodec())
    if (space == "gray" || space == "gray8")
      if (image.getCodec()->toGray(image))
	return true;
  
  int spp, bps;
  if (space == "bw" || space == "bilevel" || space == "gray1") {
    spp = 1; bps = 1;
  } else if (space == "gray2") {
    spp = 1; bps = 2;
  } else if (space == "gray4") {
    spp = 1; bps = 4;
  } else if (space == "gray" || space == "gray8") {
    spp = 1; bps = 8;
  } else if (space == "gray16") {
    spp = 1; bps = 16;
  } else if (space == "rgb" || space == "rgb8") {
    spp = 3; bps = 8;
  } else if (space == "rgb16") {
    spp = 3; bps = 16;
  // TODO: CYMK, YVU, RGBA, GRAYA...
  } else {
    std::cerr << "Requested colorspace conversion not yet implemented."
              << std::endl;
    return false;
  }

  // no image data, e.g. for loading raw images
  if (!image.getRawData()) {
    image.spp = spp;
    image.bps = bps;
    return true;
  }

  // up
  if (image.bps == 1 && bps == 2)
    colorspace_gray1_to_gray2 (image);
  else if (image.bps == 1 && bps == 4)
    colorspace_gray1_to_gray4 (image);
  else if (image.bps < 8 && bps >= 8)
    colorspace_grayX_to_gray8 (image);

  // upscale to 8 bit even for sub byte gray since we have no inter sub conv., yet
  if (image.bps < 8 && image.bps > bps)
    colorspace_grayX_to_gray8 (image);
  
  if (image.bps == 8 && image.spp == 1 && spp == 3)
    colorspace_gray8_to_rgb8 (image);

  if (image.bps == 8 && bps == 16)
    colorspace_8_to_16 (image);
  
  // down
  if (image.bps == 16 && bps < 16)
    colorspace_16_to_8 (image);

  if (image.spp == 4 && spp < 4)
    colorspace_rgba8_to_rgb8 (image);
 
  if (image.spp == 3 && spp == 1) 
    colorspace_rgb8_to_gray8 (image);

  if (spp == 1 && image.bps > bps) {
    if (image.bps == 8 && bps == 1)
      colorspace_gray8_to_gray1 (image);
    else if (image.bps == 8 && bps == 2)
      colorspace_gray8_to_gray2 (image);
    else if (image.bps == 8 && bps == 4)
      colorspace_gray8_to_gray4 (image);
  }

  if (image.spp != spp || image.bps != bps) {
    std::cerr << "Incomplete colorspace conversion. Requested: spp: "
              << spp << ", bps: " << bps
              << " - now at spp: " << image.spp << ", bps: " << image.bps
              << std::endl;
    return false;
  }
  return true;
}

// --
#include <math.h>

static inline double getExponentContrast (double contrast) {
  return (contrast < 0.0) ? 1.0 + contrast : ( (contrast == 1.0) ? 127 : 1.0 / (1.0 - contrast) );
}

static inline double getExponentGamma (double gamma) {
  return 1.0 / gamma;
}

static inline double convert (double val,
			      double brightness,
			      double contrast,
			      double gamma)
{
    /* apply brightness */
    if (brightness < 0.0)
        val = val * (1.0 + brightness);
    else if (brightness > 0.0)
        val = val + ((1.0 - val) * brightness);

    /* apply contrast */
    if (contrast != 0.0) {
      double nvalue = (val > 0.5) ? 1.0 - val : val;
      if (nvalue < 0.0)
	nvalue = 0.0;
      
      nvalue = 0.5 * pow (2.0 * nvalue, getExponentContrast (contrast));
      val = (val > 0.5) ? 1.0 - nvalue : nvalue;
    }
    
    /* apply gamma */
    if (gamma != 1.0) {
      val = pow (val, getExponentGamma (gamma));
    }
    
    return val;
}


void brightness_contrast_gamma (Image& image, double brightness, double contrast, double gamma)
{
  double r, g, b;
  
  Image::iterator end = image.end();
  for (Image::iterator it = image.begin(); it != end; ++it)
    {
      *it;
      it.getRGB (r, g, b);
      
      r = convert (r, brightness, contrast, gamma);
      g = convert (g, brightness, contrast, gamma);
      b = convert (b, brightness, contrast, gamma);
      
      it.setRGB (r, g, b);
      it.set(it);
    }
  image.setRawData();
}

void hue_saturation_lightness (Image& image, double hue, double saturation, double lightness)
{
  double h, s, v;
  
  Image::iterator end = image.end();
  for (Image::iterator it = image.begin(); it != end; ++it)
    {
      //it = it.at(371, 86);
      *it;
      it.getHSV (h, s, v);
      
      h += hue;
      
      if (h < 0.)
	h += 360.;
      else if (h >= 360.)
	h -= 360.;
      
      // TODO: this might not be accurate, double check, ...
      s = s + s * saturation;
      s = std::max (std::min (s, 1.), 0.);
       
      v = v + v * lightness;
      v = std::max (std::min (v, 1.), 0.);
      
      it.setHSV (h, s, v);
      it.set(it);
      //return;
    }
  image.setRawData();
}

void invert (Image& image)
{
  if (image.spp == 1 && image.bps == 1) {
      for (uint8_t* it = image.getRawData(); it < image.getRawDataEnd(); ++it)
        *it = *it ^ 0xFF;
      image.setRawData();
      return;
  }
  if (image.bps == 8) {
      for (uint8_t* it = image.getRawData(); it < image.getRawDataEnd(); ++it)
        *it = 0xFF - *it;
      image.setRawData();
      return;
  }
  
  double r, g, b;
  
  Image::iterator end = image.end();
  for (Image::iterator it = image.begin(); it != end; ++it)
    {
      *it;
      it.getRGB (r, g, b);

      /*inverts color*/
      r = 1.0 - r;
      g = 1.0 - g;
      b = 1.0 - b;

      it.setRGB (r, g, b);
      it.set(it);
    }
  image.setRawData();
}
