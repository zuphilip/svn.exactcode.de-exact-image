/*
 * Colorspace conversions.
 * Copyright (C) 2006 - 2016 René Rebe, ExactCODE GmbH, Germany.
 * Copyright (C) 2007 Susanne Klaus, ExactCODE GmbH, Germany.
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
#include <map>
#include <vector>

#include "Image.hh"
#include "ImageIterator2.hh"
#include "Codecs.hh"
#include "Colorspace.hh"

#include "Endianess.hh"

#include "string.h"

void realignImage(Image& image, const uint32_t newstride)
{
  const unsigned stride = image.stride();
  if (stride == newstride) return;
  
  image.getRawData(); // make sure data is already decoded
  
  if (newstride > stride)
    image.resize(image.w, image.h, newstride);
  
  uint8_t* data = image.getRawData();
  if (newstride < stride) {
    for (int y = 0; y < image.h; ++y)
      memmove(data + y * newstride, data + y * stride, newstride);
    image.resize(image.w, image.h, newstride);
  } else { // newstride > stride
    for (int y = image.h - 1; y >= 0; --y)
      memmove(data + y * newstride, data + y * stride, stride);
  }
  image.setRawData();
}

template <typename T>
struct histogram_template
{
  std::vector<std::vector<unsigned> > operator() (Image& image, int bins = 256)
  {
    std::vector<std::vector<unsigned> > hist;
    hist.resize(image.spp);
    
    typename T::accu a;
    const typename T::accu one = T::accu::one();
    
    for (int i = 0; i < image.spp; ++i)
      hist[i].resize(bins, 0);
    
    T it (image);
    for (int y = 0; y < image.h; ++y) {
      it.at(0, y);
      for (int x = 0; x < image.w; ++x)
	{
	  a = *it; ++it;
	  for (int i = 0; i < image.spp; ++i) {
	    typename T::accu::vtype v = a.v[i];
	    int j = (int)(v * (bins-1) / one.v[i]);
	    if (j < 0) j = 0;
	    else if (j >= bins) j = bins - 1;
	    hist[i][j]++;
	  }
	}
    }
    
    return hist;
  }
};

std::vector<std::vector<unsigned> > histogram(Image& image, int bins)
{
    return codegen_return<std::vector<std::vector<unsigned> >,
	histogram_template> (image, bins);
}

template <typename T>
struct normalize_template
{
  void operator() (Image& image, uint8_t l, uint8_t h)
  {
    typename T::accu a;
    typename T::accu::vtype black(0), white(0);
    
    // darkest 1%, lightest .5%
    const int white_point = image.w * image.h / 100;
    const int black_point = white_point / 2;
    
    std::vector<std::vector<unsigned> > hist = histogram(image);

    // find suitable black and white points
    {
      typedef std::vector<unsigned> histogram_type;
      
      int count = 0, ii = 0;
      for (typename histogram_type::iterator i = hist[0].begin();
	   i != hist[0].end(); ++i, ++ii) {
	int c = 0;
	for (int j = 0; j < a.samples; ++j)
	  c += hist[j][ii];
	count += c / a.samples; // unweighted average
	
	if (count > black_point) {
	  black = ii;
	  break;
	}
      }
      count = 0; ii = 255;
      for (typename histogram_type::reverse_iterator i = hist[0].rbegin();
	   i != hist[0].rend(); ++i, --ii) {
	int c = 0;
	for (int j = 0; j < a.samples; ++j)
	  c += hist[j][ii];
	count += c / a.samples; // unweighted average
	
	if (count > white_point) {
	  white = ii;
	  break;
	}
      }
    }
    
    // TODO: scale to type range
    if (l)
      black = l;
    if (h)
      white = h;
    
    typename T::accu::vtype fa = T::accu::one().v[0],
      fb = -T::accu::one().v[0] * black / 255;
    fa *= 256; // shift for interger multiplication
    fa /= T::accu::one().v[0] * (white - black) / 255;
    
    T it (image);
    for (int y = 0; y < image.h; ++y) {
      for (int x = 0; x < image.w; ++x)
	{
	  a = *it;
	  a += fb;
	  a *= fa;
	  a /= 255;
	  a.saturate();
	  it.set(a);
	  ++it;
	}
    }
    image.setRawData();
  }
};

void normalize (Image& image, uint8_t l, uint8_t h)
{
  codegen<normalize_template> (image, l, h);
}

template <typename T>
struct equalize_template
{
  void operator() (Image& image, uint8_t l, uint8_t h)
  {
    // darkest 1%, lightest .5%
    const int white_point = image.w * image.h / 100;
    const int black_point = white_point / 2;

    std::vector<std::vector<unsigned> > hist = histogram(image);
    
    typename T::accu a;
    typename T::accu::vtype blacks[a.samples] = {}, whites[a.samples] = {};
    
    // find suitable black and white points
    for (int sample = 0; sample < a.samples; ++sample) {
      typedef std::vector<unsigned> histogram_type;
      
      int count = 0, ii = 0;
      for (typename histogram_type::iterator i = hist[sample].begin();
	   i != hist[sample].end(); ++i, ++ii) {
	count += *i;
	if (count > black_point) {
	  blacks[sample] = ii;
	  break;
	}
      }
      count = 0; ii = hist[sample].size() - 1;
      for (typename histogram_type::reverse_iterator i = hist[sample].rbegin();
	   i != hist[sample].rend(); ++i, --ii) {
	count += *i;
	if (count > white_point) {
	  whites[sample] = ii;
	  break;
	}
      }
      
      // TODO: scale to type range
      if (l)
	blacks[sample] = l;
      if (h)
	whites[sample] = h;
    }
    
    typename T::accu fa = T::accu::one(), fb;
    for (int sample = 0; sample < a.samples; ++sample) {
      fb.v[sample] = -T::accu::one().v[sample] * blacks[sample] / 255;
      fa.v[sample] *= 256; // int fixed point scale
      fa.v[sample] /= T::accu::one().v[sample] * (whites[sample] - blacks[sample]) / 255;
    }
    
    T it(image);
    for (int y = 0; y < image.h; ++y) {
      it.at(0, y);
      for (int x = 0; x < image.w; ++x) {
	a = *it;
	a += fb;
	a *= fa;
	a /= 255; //T::accu::one().v[0];
	a.saturate();
	it.set(a);
	++it;
      }
    }
    image.setRawData();
  }
};

void equalize (Image& image, uint8_t l, uint8_t h)
{
  codegen<equalize_template> (image, l, h);
}

void colorspace_rgba8_to_rgb8 (Image& image)
{
  unsigned ostride = image.stride();
  image.spp = 3; image.rowstride = 0;
  
  for (int y = 0; y < image.h; ++y)
  {
    uint8_t* output = image.getRawData() + y * image.stride();
    uint8_t* it = image.getRawData() + y * ostride;
    for (int x = 0; x < image.w; ++x)
    {
      *output++ = *it++;
      *output++ = *it++;
      *output++ = *it++;
      it++; // skip over a
    }
  }

  image.resize(image.w, image.h); // realloc
}

void colorspace_argb8_to_rgb8 (Image& image)
{
  uint8_t* data = image.getRawData();
  unsigned ostride = image.stride();
  
  image.spp = 3; image.rowstride = 0;
  for (int y = 0; y < image.h; ++y)
  {
    uint8_t* output = data + y * image.stride();
    uint8_t* it = data + y * ostride;
    for (int x = 0; x < image.w; ++x)
    {
      it++; // skip over a
      *output++ = *it++;
      *output++ = *it++;
      *output++ = *it++;
    }
  }

  image.resize(image.w, image.h); // realloc
}

template<typename T, typename T2>
struct colorspace_cmyk_to_rgb_template
{
  void operator() (Image& image)
  {
    T it(image);
    image.spp = 3; // update early, for out stride
    image.rowstride = 0;
    T2 ot(image);
    
    const typename T::accu one = T::accu::one();
    for (int y = 0; y < image.h; ++y) {
      it.at(0, y);
      ot.at(0, y);
      for (int x = 0; x < image.w; ++x) {
	const typename T::accu a = *it; ++it;
	const typename T::accu::vtype
	  c = a.v[0], m = a.v[1], y = a.v[2], k = a.v[3]; 
    	
	typename T2::accu o;
    	o.v[0] = one.v[0] - std::min(c+k, one.v[0]); // ((0xff-c)*(0xff-k)) >> 8;
	o.v[1] = one.v[1] - std::min(m+k, one.v[1]); // ((0xff-m)*(0xff-k)) >> 8;
	o.v[2] = one.v[2] - std::min(y+k, one.v[2]); // ((0xff-y)*(0xff-k)) >> 8;
	ot.set(o); ++ot;
      }
    }
    
    image.resize(image.w, image.h); // realloc
  }
};

void colorspace_cmyk_to_rgb(Image& image)
{
  // manual codegen for the only colorspaces we care about
  // we misuse the rgba iterators for now, ...
  // codegen<colorspace_cmyk_to_rgb_template> (image);
  if (image.bps == 16) {
    colorspace_cmyk_to_rgb_template<rgba16_iterator, rgb16_iterator> a;
    a (image);
  } else {
    colorspace_cmyk_to_rgb_template<rgba_iterator, rgb_iterator> a;
    a (image);
  }
}

void colorspace_rgb8_to_gray8 (Image& image, const int bytes, const int wR, const int wG, const int wB)
{
  unsigned ostride = image.stride();
  image.spp = 1; image.rowstride = 0;
  
  const int sum = wR + wG + wB;
  uint8_t* data = image.getRawData();
  for (int y = 0; y < image.h; ++y)
  {
    uint8_t* output = data + y * image.stride();
    uint8_t* it = data + y * ostride;
    for (int x = 0; x < image.w; ++x, it += bytes)
    {
      // R G B order and associated weighting
      int c  = wR * it[0] + wG * it[1] + wB * it[2];
      *output++ = (uint8_t)(c / sum);
    }
  }
  
  image.resize(image.w, image.h); // realloc
}

void colorspace_rgb16_to_gray16 (Image& image, const int wR = 30, const int wG = 59, const int wB = 11)
{
  unsigned ostride = image.stride();
  image.spp = 1; image.rowstride = 0;
  unsigned stride = image.stride();

  uint8_t* data = image.getRawData();
  const int sum = wR + wG + wB;
  for (int y = 0; y < image.h; ++y)
  {
    uint16_t* output = (uint16_t*)(data + y * stride);
    uint16_t* it = (uint16_t*)(data + y * ostride);
    for (int x = 0; x < image.w; ++x)
    {
      // R G B order and associated weighting
      int c = (int)*it++ * wR;
      c += (int)*it++ * wG;
      c += (int)*it++ * wB;
      
      *output++ = (uint16_t)(c / sum);
    }
  }
  
  image.resize(image.w, image.h); // realloc
}

void colorspace_rgb8_to_rgb8a (Image& image, uint8_t alpha)
{
  unsigned stride = image.stride();
  unsigned newstride = 4 * stride / 3;
  unsigned width = image.w;

  uint8_t* data = (uint8_t*)realloc(image.getRawData(), newstride * image.h);
  image.setRawDataWithoutDelete(data);
  image.setSamplesPerPixel(4);
  
  // reverse copy with alpha fill inside the buffer
  for (int y = image.h-1; y >= 0; --y) {
    uint8_t* it_src = data + y * stride + width * 3 - 1;
    for (uint8_t* it_dst = data + y * newstride + width * 4 - 1; it_dst > data + y * stride;) {
      *it_dst-- = alpha;
      *it_dst-- = *it_src--;
      *it_dst-- = *it_src--;
      *it_dst-- = *it_src--;
    }
  }
}

void colorspace_gray8_threshold (Image& image, uint8_t threshold)
{
  uint8_t* it = image.getRawData();
  for (int y = 0; y < image.h; ++y, it += image.stride())
  {
    for (int x = 0; x < image.w; ++x)
     it[x] = it[x] > threshold ? 0xFF : 0x00;
  }

  image.setRawData();
}

void colorspace_gray8_denoise_neighbours (Image &image, bool gross)
{
  if (image.bps != 8 || image.spp != 1)
    return;
  
  unsigned stride = image.stride();
  uint8_t* data = image.getRawData();
  uint8_t* ndata = (uint8_t*)malloc(stride * image.h);
  
  struct compare_and_set
  {
    const Image& image;
    const signed stride; // negative - indexing
    
    compare_and_set (const Image& _image)
      : image(_image), stride(_image.stride())
    {
    }
    
    // without the inner(area) compiler guidance the conditionals are
    // not optimized away well enough
    void operator() (const int x, const int y, uint8_t* it, uint8_t* it2,
		     const bool inner, const bool gross = false)
    {
      int n = 0, sum = 0;
      
      // +
      if (inner || x > 0)
	sum += it[-1], ++n;
      if (inner || y > 0)
	sum += it[-stride], ++n;
      
      if (inner || x < image.w-1)
	sum += it[1], ++n;
      if (inner || y < image.h-1)
	sum += it[stride], ++n;

      // x
      if (gross) {
	if (inner || y > 0) {
	  if (inner || x > 0)
	    sum += it[-stride - 1], ++n;
	  if (inner || x < image.w-1)
	    sum += it[-stride + 1], ++n;
	}
	
	if (inner || y < image.h-1) {
	  if (inner || x > 0)
	    sum += it[stride - 1], ++n;
	  if (inner || x < image.w-1)
	    sum += it[stride + 1], ++n;
	}
      }
      
      // if all direct neighbours are black or white, fill it
      if (gross) {
        if (sum <= 1 * 0xff)
	  *it2 = 0;
        else if (sum >= (n - 1) * 0xff)
	  *it2 = 0xff;
	else
	  *it2 = *it;
      } else {
        if (sum == 0)
	  *it2 = 0;
        else if (sum == n * 0xff)
	  *it2 = 0xff;
	else
	  *it2 = *it;
      }
    }
  } compare_and_set (image);
  
  for (int y = 0; y < image.h; ++y)
    {
      uint8_t* it = data + y * stride;
      uint8_t* it2 = ndata + y * stride;
	  
      // optimize conditionals away for the inner area
      if (y > 0 && y < image.h-1)
	{
	  compare_and_set (0, y, it++, it2++, false, gross);
	  for (int x = 1; x < image.w-1; ++x)
	    compare_and_set (x, y, it++, it2++, true, gross);
	  compare_and_set (image.w-1, y, it++, it2++, false, gross);
	}
      else // quite some out of bounds conditions to check
	for (int x = 0; x < image.w; ++x) 
	  compare_and_set (x, y, it++, it2++, false, gross);
    }
    
  image.setRawData(ndata);
}

void colorspace_gray8_to_gray1 (Image& image, uint8_t threshold)
{
  unsigned ostride = image.stride();
  image.bps = 1; image.rowstride = 0;

  for (int row = 0; row < image.h; row++)
    {
      uint8_t *output = image.getRawData() + row * image.stride();
      uint8_t *input = image.getRawData() + row * ostride;

      uint8_t z = 0;
      int x = 0;
      for (; x < image.w; ++x)
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
  
  image.resize(image.w, image.h); // realloc
}

void colorspace_gray8_to_gray4 (Image& image)
{
  unsigned ostride = image.stride();
  image.bps = 4; image.rowstride = 0;
  
  for (int row = 0; row < image.h; row++)
    {
      uint8_t *output = image.getRawData() + row * image.stride();
      uint8_t *input = image.getRawData() + row * ostride;
  
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
  
  image.resize(image.w, image.h); // realloc
}

void colorspace_gray8_to_gray2 (Image& image)
{
  unsigned ostride = image.stride();
  image.bps = 2; image.rowstride = 0;
  
  for (int row = 0; row < image.h; ++row)
    {
      uint8_t *output = image.getRawData() + row * image.stride();
      uint8_t *input = image.getRawData() + row * ostride;

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
  
  image.resize(image.w, image.h); // realloc
}

void colorspace_gray8_to_rgb8 (Image& image)
{
  const unsigned stride = image.stride();
  const unsigned nstride = image.w * 3;
  image.setRawDataWithoutDelete((uint8_t*)realloc(image.getRawData(),
						  std::max(stride, nstride) * image.h));
  uint8_t* data = image.getRawData();
  uint8_t* output = data + image.h * nstride - 1;
  for (int y = image.h - 1; y >= 0; --y)
    {
      uint8_t* it = data + y * stride;
      for (int x = image.w - 1; x >= 0; --x)
	{
	  *output-- = it[x];
	  *output-- = it[x];
	  *output-- = it[x];
	}
    }
  
  image.spp = 3;
  image.resize(image.w, image.h); // realloc
}

void colorspace_grayX_to_gray8 (Image& image)
{
  uint8_t* old_data = image.getRawData();
  unsigned old_stride = image.stride();
  
  const int bps = image.bps;
  image.bps = 8; image.rowstride = 0;
  image.setRawDataWithoutDelete ((uint8_t*)malloc(image.h * image.stride()));
  uint8_t* output = image.getRawData();
  
  const int vmax = 1 << bps;
#ifdef _MSC_VER
  std::vector<uint8_t> gray_lookup(vmax);
#else
  uint8_t gray_lookup[vmax];
#endif
  for (int i = 0; i < vmax; ++i) {
    gray_lookup[i] = 0xff * i / (vmax - 1);
    //std::cerr << i << " = " << (int)gray_lookup[i] << std::endl;
  }
  
  const unsigned int bitshift = 8 - bps;
  for (int row = 0; row < image.h; ++row)
    {
      uint8_t* input = old_data + row * old_stride;
      uint8_t z = 0, bits = 0;
      
      for (int x = 0; x < image.w; ++x)
	{
	  if (bits == 0) {
	    z = *input++;
	    bits = 8;
	  }
	  
	  *output++ = gray_lookup[z >> bitshift];
	  
	  z <<= bps;
	  bits -= bps;
	}
    }
  free (old_data);
}

void colorspace_grayX_to_rgb8 (Image& image)
{
  uint8_t* old_data = image.getRawData();
  unsigned old_stride = image.stride();
  
  const int bps = image.bps;
  image.spp = 3;
  image.bps = 8; image.rowstride = 0;
  image.setRawDataWithoutDelete ((uint8_t*)malloc(image.h * image.stride()));
  uint8_t* output = image.getRawData();
  
  const int vmax = 1 << bps;
#ifdef _MSC_VER
  std::vector<uint8_t> gray_lookup(vmax);
#else
  uint8_t gray_lookup[vmax];
#endif
  for (int i = 0; i < vmax; ++i) {
    gray_lookup[i] = 0xff * i / (vmax - 1);
    //std::cerr << i << " = " << (int)gray_lookup[i] << std::endl;
  }
  
  const unsigned int bitshift = 8 - bps;
  for (int row = 0; row < image.h; ++row)
    {
      uint8_t* input = old_data + row * old_stride;
      uint8_t z = 0;
      unsigned int bits = 0;
      
      for (int x = 0; x < image.w; ++x)
	{
	  if (bits == 0) {
	    z = *input++;
	    bits = 8;
	  }
	  
	  *output++ = gray_lookup[z >> bitshift];
	  *output++ = gray_lookup[z >> bitshift];
	  *output++ = gray_lookup[z >> bitshift];
	  
	  z <<= bps;
	  bits -= bps;
	}
    }
  free (old_data);
}

void colorspace_gray1_to_gray2 (Image& image)
{
  uint8_t* old_data = image.getRawData();
  unsigned old_stride = image.stride();
  
  image.bps = 2;
  image.rowstride = 0;
  image.setRawDataWithoutDelete ((uint8_t*)malloc(image.h * image.stride()));
  uint8_t* output = image.getRawData();
  
  for (int row = 0; row < image.h; ++row)
    {
      uint8_t z = 0;
      uint8_t zz = 0;
      uint8_t* input = old_data + row * old_stride;

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
  unsigned old_stride = image.stride();
  
  image.bps = 4; image.rowstride = 0;
  image.setRawDataWithoutDelete ((uint8_t*)malloc(image.h * image.stride()));
  uint8_t* output = image.getRawData();
  
  for (int row = 0; row < image.h; ++row)
    {
      uint8_t z = 0;
      uint8_t zz = 0;
      
      uint8_t* input = old_data + row * old_stride;
      
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

void colorspace_16_to_8 (Image& image)
{
  uint8_t* output = image.getRawData();
  unsigned ostride = image.stride();
  image.bps = 8; image.rowstride = 0;
  
  for (int y = 0; y < image.h; ++y)
    {
      uint16_t* it = (uint16_t*)(image.getRawData() + y * ostride);
      for (unsigned x = 0; x < image.stride(); ++x)
	{
	  *output++ = it[x] >> 8;
	}
    }
  
  image.resize(image.w, image.h); // realloc
}

void colorspace_8_to_16 (Image& image)
{
  unsigned stride = image.stride();
  image.setRawDataWithoutDelete((uint8_t*)realloc(image.getRawData(),
						  stride * 2 * image.h));
  
  uint8_t* data = image.getRawData();
  for (int y = image.h - 1; y >= 0; --y)
    {
      uint8_t* data8 = data + y * stride;
      uint16_t* data16 = (uint16_t*)(data + y * stride * 2);
      
      for (int x = stride - 1; x >= 0; --x)
	{
	  data16[x] = data8[x] * 0xffff / 255;
	}
    }
  
  image.rowstride = stride * 2;
  image.bps = 16; // converted 16bit data
}

void colorspace_de_ieee (Image& image)
{
  typedef uint8_t value_t;
  value_t* data = (value_t*)image.getRawData();
  
  switch (image.bps) {
  case 32:
    {
      float* fp32 = (float*)data;
      for (int i = 0; i < image.w * image.h * image.spp; ++i)
	data[i] = (value_t)std::max(std::min(fp32[i], (float)255), (float)0);
      image.bps = sizeof(value_t) * 8;
      image.setRawData();
    }
    break;
    
  case 64:
    {
      double* fp64 = (double*)data;
      for (int i = 0; i < image.w * image.h * image.spp; ++i)
	data[i] = (value_t)std::max(std::min(fp64[i], (double)255), (double)0);
      image.bps = sizeof(value_t) * 8;
      image.setRawData();
    }
    break;
    
  default:
    std::cerr << "colorspace_de_ieee: unsupported bps: " << image.bps << std::endl;
  }
}

void colorspace_de_palette (Image& image, int table_entries,
			    uint16_t* rmap, uint16_t* gmap, uint16_t* bmap, uint16_t* amap)
{
  // detect 1bps b/w tables
  if (image.bps == 1 && table_entries >= 2 && !amap) {
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
  if (table_entries > 1 && !amap) {
    bool is_ordered_gray = (image.bps == 8 || image.bps == 4 ||
			    image.bps == 2) && (1 << image.bps == table_entries);
    is_gray = true;
    
    // std::cerr << (1 << image.bps) << " vs " << table_entries << std::endl;
    
    // std::cerr << "checking for gray table" << std::endl;
    for (int i = 0; (is_gray || is_ordered_gray) && i < table_entries; ++i) {
      // std::cerr << rmap[i] << " " << gmap[i] << " " << bmap[i] << std::endl;
      if (rmap[i] >> 8 != gmap[i] >> 8 ||
	  rmap[i] >> 8 != bmap[i] >> 8) {
	is_gray = is_ordered_gray = false;
      }
      else if (is_ordered_gray) {
	const int ref = i * 0xff / (table_entries - 1);
	if (rmap[i] >> 8 != ref ||
	    gmap[i] >> 8 != ref ||
	    bmap[i] >> 8 != ref)
	  is_ordered_gray = false;
      }
    }
    
    // std::cerr << "gray: " << is_gray << ", is ordered: " << is_ordered_gray << std::endl;
    
    if (is_ordered_gray)
      return;
  }
  
  int new_size = image.w * image.h;
  if (amap)
    new_size *= 4; // RGBA, CMYK
  else if (!is_gray) // RGB
    new_size *= 3;
  
  uint8_t* orig_data = image.getRawData();
  uint32_t orig_stride = image.stride();
  uint8_t* new_data = (uint8_t*)malloc(new_size);
  uint8_t* dst = new_data;
  
  // TODO: allow 16bit output if the palette contains that much dynamic
  
  const unsigned bps = image.bps;
  const unsigned bitshift = bps < 8 ? 8 - bps : 0;
  for (int y = 0; y < image.h; ++y) {
    uint8_t* src = orig_data + y * orig_stride;
    uint8_t z = 0;
    uint16_t v;
    unsigned bits = 0;
    
    for (int x = 0; x < image.w; ++x){
      if (bps > 8) {
	v = *(uint16_t*)src;
	src += 2;
      }
      else {
	if (bits == 0) {
	  z = *src++;
	  bits = 8;
	}
	v = z >> bitshift;
	z <<= bps;
	bits -= bps;
      }
      
      if (is_gray) {
	*dst++ = rmap[v] >> 8;
      } else {
	*dst++ = rmap[v] >> 8;
	*dst++ = gmap[v] >> 8;
	*dst++ = bmap[v] >> 8;
	if (amap) *dst++ = amap[v >> 8];
      }
    }
  }

  image.bps = 8;
  if (is_gray)
    image.spp = 1;
  else if (amap)
    image.spp = 4;
  else
    image.spp = 3;
  
  image.rowstride = 0;
  image.setRawData(new_data);

  // special case, e.g. for 1-bit XPM
  if (is_gray && table_entries == 2) {
    if (rmap[0] == 0 &&
        gmap[0] == 0 &&
        bmap[0] == 0 &&
        rmap[1] >= 0xff00 &&
        gmap[1] >= 0xff00 &&
        bmap[1] >= 0xff00)
      {
       	//std::cerr << "b/w after all." << std::endl;
        colorspace_by_name(image, "bw");
      }
  }
}

bool colorspace_by_name (Image& image, const std::string& target_colorspace,
			 uint8_t threshold)
{
  std::string space = target_colorspace;
  std::transform (space.begin(), space.end(), space.begin(), tolower);
    
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
  } else if (space == "rgba" || space == "rgba8") {
    spp = 4; bps = 8;
  } else if (space == "rgb16") {
    spp = 3; bps = 16;
  // TODO: CYMK, YVU, RGBA, GRAYA...
  } else {
    std::cerr << "Requested colorspace conversion not yet implemented."
              << std::endl;
    return false;
  }

  return colorspace_convert(image, spp, bps, threshold);
}

bool colorspace_convert(Image& image, int spp, int bps, uint8_t threshold)
{
  // thru the codec?
  if (!image.isModified() && image.getCodec())
    if (spp == 1 && bps >= 8)
      if (image.getCodec()->toGray(image))
	return true;

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
  if (image.bps < 8 && image.bps != bps)
    colorspace_grayX_to_gray8 (image);
  
  if (image.bps == 8 && image.spp == 1 && spp >= 3)
    colorspace_gray8_to_rgb8 (image);

  if (image.bps == 8 && bps == 16)
    colorspace_8_to_16 (image);
  
  // down
  if (image.bps == 16 && bps < 16)
    colorspace_16_to_8 (image);
  
  if (image.spp == 4 && spp < 4 && image.bps == 8) { // TODO: might be RGB16
    if (spp < 3)
      colorspace_rgb8_to_gray8 (image, 4);
    else
      colorspace_rgba8_to_rgb8 (image);
  }

  if (image.spp == 3 && spp == 4 && image.bps == 8) { // TODO: might be RGB16
      colorspace_rgb8_to_rgb8a (image);
  }
  
  if (image.spp == 3 && spp == 1) {
    if (image.bps == 8) 
      colorspace_rgb8_to_gray8 (image);
    else if (image.bps == 16)
      colorspace_rgb16_to_gray16 (image);
  }
  
  if (spp == 1 && image.bps > bps) {
    if (image.bps == 8 && bps == 1)
      colorspace_gray8_to_gray1 (image, threshold);
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
    image.spp = spp;
    image.bps = bps;
    image.resize(image.w, image.h);
    
    return false;
  }
  return true;
}

const char* colorspace_name (Image& image)
{
  switch (image.spp * image.bps)
    {
    case 1: return "gray1";
    case 2: return "gray2";
    case 4: return "gray4";
    case 8: return "gray8";
    case 16: return "gray16";
    case 24: return "rgb8";
    case 32: return "rgba8";
    case 48: return "rgb16";
    default: return "";
    }
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
    // apply brightness
    if (brightness < 0.0)
        val = val * (1.0 + brightness);
    else if (brightness > 0.0)
        val = val + ((1.0 - val) * brightness);

    // apply contrast
    if (contrast != 0.0) {
      double nvalue = (val > 0.5) ? 1.0 - val : val;
      if (nvalue < 0.0)
	nvalue = 0.0;
      
      nvalue = 0.5 * pow (2.0 * nvalue, getExponentContrast (contrast));
      val = (val > 0.5) ? 1.0 - nvalue : nvalue;
    }
    
    // apply gamma
    if (gamma != 1.0) {
      val = pow (val, getExponentGamma (gamma));
    }
    
    return val;
}

template<typename T>
struct brightness_contrast_gamma_template
{
  void operator() (Image& image, double brightness, double contrast, double gamma)
  {
    T it (image);
    typename T::accu a;
    typename T::accu::vtype _r, _g, _b;
    double r, g, b;
    
    for (int y = 0; y < image.h; ++y) {
      it.at(0, y);
      for (int x = 0; x < image.w; ++x) {
	a = *it;
	
	a.getRGB (_r, _g, _b);
	r = _r, g = _g, b = _b;
	r /= T::accu::one().v[0];
	g /= T::accu::one().v[0];
	b /= T::accu::one().v[0];
	
	r = convert (r, brightness, contrast, gamma);
	g = convert (g, brightness, contrast, gamma);
	b = convert (b, brightness, contrast, gamma);
	
	_r = (typename T::accu::vtype)(r * T::accu::one().v[0]);
	_g = (typename T::accu::vtype)(g * T::accu::one().v[0]);
	_b = (typename T::accu::vtype)(b * T::accu::one().v[0]);
	a.setRGB (_r, _g, _b);
	it.set(a);
	++it;
      }
    }
    image.setRawData();
  }
};

void brightness_contrast_gamma (Image& image, double brightness, double contrast, double gamma)
{
  codegen<brightness_contrast_gamma_template> (image, brightness, contrast, gamma);
}

template <typename T>
struct hue_saturation_lightness_template {
  void operator() (Image& image, double _hue, double saturation, double lightness)
  {
    T it (image);

    // optimized ONE2 in divisions imprecise shifts if not an FP type
    const typename T::accu::vtype ONE = T::accu::one().v[0],
                                  ONE2 = ONE > 1 ? ONE + 1 : ONE;

    // H in degree, S and L [-1, 1], latér scaled to the integer range
    _hue = fmod (_hue, 360);
    if (_hue < 0)
      _hue += 360;
    const typename T::accu::vtype
      hue = ONE * _hue / 360;

    for (int y = 0; y < image.h; ++y) {
      it.at(0, y);
      for (int x = 0; x < image.w; ++x) {
	typename T::accu a = *it;	
	typename T::accu::vtype r, g, b;
	a.getRGB (r, g, b);

	// RGB to HSV
	typename T::accu::vtype h, s, v;
	{
	  const typename T::accu::vtype min = std::min (std::min (r, g), b);
	  const typename T::accu::vtype max = std::max (std::max (r, g), b);
	  const typename T::accu::vtype delta = max - min;
	  v = max;

	  if (delta == 0) {
	    h = 0;
	    s = 0;
	  }
	  else {
	    s = max == 0 ? 0 : ONE - (ONE * min / max);
	    
	    if (max == r) // yellow - magenta
	      h = (60*ONE/360) * (g - b) / delta + (g >= b ? 0 : ONE);
	    else if (max == g) // cyan - yellow
	      h = (60*ONE/360) * (b - r) / delta + 120*ONE/360;
	    else // magenta - cyan
	      h = (60*ONE/360) * (r - g) / delta + 240*ONE/360;
	  }
	}
	
	// hue should only be positive, se we only need to check one overflow
	h += hue;
	if (h >= ONE)
	  h -= ONE;
	
	// TODO: this might not be accurate, double check, ...
	s = s + s * saturation;
	s = std::max (std::min (s, ONE), (typename T::accu::vtype)0);
	
	v = v + v * lightness;
	v = std::max (std::min (v, ONE), (typename T::accu::vtype)0);
	
	// back from HSV to RGB
	{
	  const int i = 6 * h / ONE2;
	  
	  const typename T::accu::vtype f = 6 * h % ONE2;
	  // only compute the ones "on-demand" as needed
	  #define p (v * (ONE - s) / ONE2)
	  #define q (v * (ONE - f * s / ONE2) / ONE2)
	  #define t (v * (ONE - (ONE - f) * s / ONE2) / ONE2)

	  switch (i) {
	  case 0:
	    r = v;
	    g = t;
	    b = p;
	    break;
	  case 1:
	    r = q;
	    g = v;
	    b = p;
	    break;
	  case 2:
	    r = p;
	    g = v;
	    b = t;
	    break;
	  case 3:
	    r = p;
	    g = q;
	    b = v;
	    break;
	  case 4:
	    r = t;
	    g = p;
	    b = v;
	    break;
	  default: // case 5:
	    r = v;
	    g = p;
	    b = q;
	    break;
	  }
#undef p
#undef q
#undef t
	} // end

	a.setRGB (r, g, b);
	it.set(a);
	++it;
      }
    }
    image.setRawData();
  }
};

void hue_saturation_lightness (Image& image, double hue, double saturation, double lightness)
{
  codegen<hue_saturation_lightness_template> (image, hue, saturation, lightness);
}

template <typename T>
struct invert_template
{
  void operator() (Image& image)
  {
    T it (image);
    for (int y = 0; y < image.h; ++y) {
      it.at(0, y);
      for (int x = 0; x < image.w; ++x) {
	
	typename T::accu a = *it;
	a = T::accu::one() -= a;
	it.set (a);
	++it;
      }
    }
    image.setRawData();
  }
};

void invert (Image& image)
{
  codegen<invert_template> (image);
}

template <typename T>
struct colorspace_pack_line_template
{
  void operator() (Image& image, int dstline, int srcline)
  {
    T dst(image);
    T src(image);
    dst.at(0, dstline);
    src.at(0, srcline);

    typename T::type* scratch = src.ptr;
    
    for (int x = 0; x < image.w; ++x) {
      typename T::accu a;
      for (int s = 0; s < a.samples; ++s)
	a.v[s] = scratch[s * image.w + x];
      
      dst.set(a);
      ++dst;
    }
  }
};

void colorspace_pack_line (Image& image, int dst, int src)
{
  codegen<colorspace_pack_line_template> (image, dst, src);
}
