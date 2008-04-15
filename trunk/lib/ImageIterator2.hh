/*
 * Copyright (C) 2008 René Rebe, ExactCODE GmbH Germany.
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

#include "Image.hh"

class rgb_iterator
{
public:
  uint8_t* ptr;
  uint8_t* ptr_begin;
  const Image& image;
  const int stride;
    
  class accu
  {
  public:
    unsigned int v1, v2, v3;
      
    accu& operator*= (int f) {
      v1 *= f;
      v2 *= f;
      v3 *= f;
      return *this;
    }

    accu operator* (int f) {
      accu a = *this;
      return a *= f;
    }
      
    accu& operator/= (int f) {
      v1 /= f;
      v2 /= f;
      v3 /= f;
      return *this;
    }
      
    accu& operator+= (const accu& other) {
      v1 += other.v1;
      v2 += other.v2;
      v3 += other.v3;
      return *this;
    }


    accu& operator= (const Image::iterator& background)
    {
      double r, g, b;
      background.getRGB(r, g, b);
      v1 = r * 255;
      v2 = g * 255;
      v3 = b * 255;
      return *this;
    }
  };
    
  rgb_iterator (Image& _image)
    : ptr_begin(_image.getRawData()), image (_image), stride(_image.stride()) {
    ptr = ptr_begin;
  }
    
  rgb_iterator& at (int x, int y) {
    ptr = ptr_begin + y * stride + x * 3;
    return *this;
  }
    
  rgb_iterator& operator++ () {
    ptr += 3;
    return *this;
  }
    
  accu operator* () {
    accu a;
    a.v1 = ptr[0];
    a.v2 = ptr[1];
    a.v3 = ptr[2];
    return a;
  }
    
  rgb_iterator& set (const accu& a) {
    ptr[0] = a.v1;
    ptr[1] = a.v2;
    ptr[2] = a.v3;
    return *this;
  }
    
};

class gray_iterator
{
public:
  uint8_t* ptr;
  uint8_t* ptr_begin;
  const Image& image;
  const int stride;
    
  class accu
  {
  public:
    unsigned int v1;
      
    accu& operator*= (int f) {
      v1 *= f;
      return *this;
    }

    accu operator* (int f) {
      accu a = *this;
      return a *= f;
    }
      
    accu& operator/= (int f) {
      v1 /= f;
      return *this;
    }
      
    accu& operator+= (const accu& other) {
      v1 += other.v1;
      return *this;
    }
      
    accu& operator= (const Image::iterator& background)
    {
      v1 = background.getL();
      return *this;
    }
      
  };
    
  gray_iterator (Image& _image)
    : ptr_begin(_image.getRawData()), image (_image), stride(_image.stride()) {
    ptr = ptr_begin;
  }
    
  gray_iterator& at (int x, int y) {
    ptr = ptr_begin + y * stride + x;
    return *this;
  }
    
  gray_iterator& operator++ () {
    ptr += 1;
    return *this;
  }
    
  accu operator* () {
    accu a;
    a.v1 = ptr[0];
    return a;
  }
    
  gray_iterator& set (const accu& a) {
    ptr[0] = a.v1;
    return *this;
  }
    
};

/* template arguments to create an specialized instance for
   each bit-pattern */

class bitdepth1
{
public:
  const static unsigned int bitdepth = 1;
};
class bitdepth2
{
public:
  const static unsigned int bitdepth = 2;
};
class bitdepth4
{
public:
  const static unsigned int bitdepth = 4;
};
  
template <typename T>
class bit_iterator
{
public:
  uint8_t* ptr;
  uint8_t* ptr_begin;
  int _x;
  const Image& image;
  const int width, stride;
  int bitpos;
  const static unsigned int bitdepth = T::bitdepth;
  const int mask;

  typedef gray_iterator::accu accu; // reuse
    
  bit_iterator (Image& _image)
    : ptr_begin(_image.getRawData()), _x(0), image (_image),
      width(_image.width()), stride(_image.stride()),
      bitpos(7), mask ((1 << bitdepth) - 1) {
    ptr = ptr_begin;
  }
    
  bit_iterator& at (int x, int y) {
    _x = x;
    ptr = ptr_begin + y * stride + x / (8 / bitdepth);
    bitpos = 7 - (x % (8 / bitdepth)) * bitdepth;
    return *this;
  }
    
  bit_iterator& operator++ () {
    ++_x;
    bitpos -= bitdepth;
    if (bitpos < 0 || _x == width) {
      if (_x == width)
	_x = 0;
      ++ptr;
      bitpos = 7;
    }
    return *this;
  }
    
  accu operator* () {
    accu a;
    a.v1 = ((*ptr >> (bitpos - (bitdepth - 1))) & mask) * 255 / mask;
    return a;
  }
    
  bit_iterator& set (const accu& a) {
    *ptr &= ~(mask << (bitpos - (bitdepth - 1)));
    *ptr |= (a.v1 >> (8 - bitdepth)) << (bitpos - (bitdepth - 1));
      
    return *this;
  }
};
