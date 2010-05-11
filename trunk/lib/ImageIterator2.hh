/*
 * Copyright (C) 2008 - 2010 René Rebe, ExactCODE GmbH Germany.
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

#include <algorithm>

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
    typedef int32_t vtype;
    vtype v1, v2, v3;
    
    accu () { v1 = v2 = v3 = 0; }
    
    static accu one () {
      accu a;
      a.v1 = a.v2 = a.v3 = 0xff;
      return a;
    }

    accu& abs() {
      v1 = std::abs(v1);
      v2 = std::abs(v2);
      v3 = std::abs(v3);
      return *this;
    }
    
    void saturate () {
      v1 = std::min (std::max (v1, (vtype)0), (vtype)0xff);
      v2 = std::min (std::max (v2, (vtype)0), (vtype)0xff);
      v3 = std::min (std::max (v3, (vtype)0), (vtype)0xff);
    }
    
    accu& operator*= (vtype f) {
      v1 *= f;
      v2 *= f;
      v3 *= f;
      return *this;
    }

    accu operator* (vtype f) const {
      accu a = *this;
      return a *= f;
    }
    
    accu& operator+= (vtype f) {
      v1 += f;
      v2 += f;
      v3 += f;
      return *this;
    }

    accu operator+ (vtype f) const {
      accu a = *this;
      return a += f;
    }
      
    accu& operator/= (vtype f) {
      v1 /= f;
      v2 /= f;
      v3 /= f;
      return *this;
    }

    accu operator/ (vtype f) const {
      accu a = *this;
      a /= f;
      return a;
    }
    
    accu& operator+= (const accu& other) {
      v1 += other.v1;
      v2 += other.v2;
      v3 += other.v3;
      return *this;
    }
    
    accu operator+ (const accu& other) const {
      accu a = *this;
      a += other;
      return a;
    }

    accu& operator-= (const accu& other) {
      v1 -= other.v1;
      v2 -= other.v2;
      v3 -= other.v3;
      return *this;
    }
    
    accu& operator= (const Image::iterator& background)
    {
      double r = 0, g = 0, b = 0;
      background.getRGB(r, g, b);
      v1 = (vtype)(r * 0xff);
      v2 = (vtype)(g * 0xff);
      v3 = (vtype)(b * 0xff);
      return *this;
    }
    
    void getRGB (vtype& r, vtype& g, vtype& b) {
      r = v1; g = v2; b = v3;
    }
    
    void getL (vtype& l) {
      l = (11 * v1 + 16 * v2 + 5 * v3) / 32;
    }
    
    void setRGB (vtype r, vtype g, vtype b) {
      v1 = r; v2 = g; v3 = b;
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

  rgb_iterator& operator-- () {
    ptr -= 3;
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

class rgba_iterator
{
public:
  uint8_t* ptr;
  uint8_t* ptr_begin;
  const Image& image;
  const int stride;
  
  class accu
  {
  public:
    typedef int32_t vtype;
    vtype v1, v2, v3, v4;
    
    accu () { v1 = v2 = v3 = v4 = 0; }
    
    static accu one () {
      accu a;
      a.v1 = a.v2 = a.v3 = a.v4 = 0xff;
      return a;
    }
    
    accu& abs() {
      v1 = std::abs(v1);
      v2 = std::abs(v2);
      v3 = std::abs(v3);
      v4 = std::abs(v4);
      return *this;
    }

    void saturate () {
      v1 = std::min (std::max (v1, (vtype)0), (vtype)0xff);
      v2 = std::min (std::max (v2, (vtype)0), (vtype)0xff);
      v3 = std::min (std::max (v3, (vtype)0), (vtype)0xff);
      v4 = std::min (std::max (v4, (vtype)0), (vtype)0xff);
    }
    
    accu& operator*= (vtype f) {
      v1 *= f;
      v2 *= f;
      v3 *= f;
      v4 *= f;
      return *this;
    }

    accu operator* (vtype f) const {
      accu a = *this;
      return a *= f;
    }
    
    accu& operator+= (vtype f) {
      v1 += f;
      v2 += f;
      v3 += f;
      v4 += f;
      return *this;
    }
    
    accu operator+ (vtype f) const {
      accu a = *this;
      return a += f;
    }
      
    accu& operator/= (vtype f) {
      v1 /= f;
      v2 /= f;
      v3 /= f;
      v4 /= f;
      return *this;
    }
    
    accu operator/ (vtype f) const {
      accu a = *this;
      a /= f;
      return a;
    }
    
    accu& operator+= (const accu& other) {
      v1 += other.v1;
      v2 += other.v2;
      v3 += other.v3;
      v4 += other.v4;
      return *this;
    }
    
    accu operator+ (const accu& other) const {
      accu a = *this;
      a += other;
      return a;
    }

    accu& operator-= (const accu& other) {
      v1 -= other.v1;
      v2 -= other.v2;
      v3 -= other.v3;
      v4 -= other.v4;
      return *this;
    }
    
    accu& operator= (const Image::iterator& background)
    {
      double r = 0, g = 0, b = 0, a = 0;
      background.getRGBA(r, g, b, a);
      v1 = (vtype)(r * 0xff);
      v2 = (vtype)(g * 0xff);
      v3 = (vtype)(b * 0xff);
      v4 = (vtype)(a * 0xff);
      return *this;
    }
    
    void getRGB (vtype& r, vtype& g, vtype& b) {
      r = v1; g = v2; b = v3;
    }
    
    void getL (vtype& l) {
      l = (11 * v1 + 16 * v2 + 5 * v3) / 32;
    }
    
    void setRGB (vtype r, vtype g, vtype b) {
      v1 = r; v2 = g; v3 = b; v3 = 0xff;
    }
  };
  
  rgba_iterator (Image& _image)
    : ptr_begin(_image.getRawData()), image (_image), stride(_image.stride()) {
    ptr = ptr_begin;
  }
    
  rgba_iterator& at (int x, int y) {
    ptr = ptr_begin + y * stride + x * 4;
    return *this;
  }
    
  rgba_iterator& operator++ () {
    ptr += 4;
    return *this;
  }

  rgba_iterator& operator-- () {
    ptr -= 4;
    return *this;
  }
  
  accu operator* () {
    accu a;
    a.v1 = ptr[0];
    a.v2 = ptr[1];
    a.v3 = ptr[2];
    a.v4 = ptr[3];
    return a;
  }
    
  rgba_iterator& set (const accu& a) {
    ptr[0] = a.v1;
    ptr[1] = a.v2;
    ptr[2] = a.v3;
    ptr[3] = a.v4;
    return *this;
  }
};

class rgb16_iterator
{
public:
  uint16_t* ptr;
  uint16_t* ptr_begin;
  const Image& image;
  const int stride;

    
  class accu
  {
  public:
    typedef int64_t vtype;
    vtype v1, v2, v3;
    
    accu () { v1 = v2 = v3 = 0; }
    
    static accu one () {
      accu a;
      a.v1 = a.v2 = a.v3 = 0xffff;
      return a;
    }
    
    accu& abs() {
      v1 = std::abs(v1);
      v2 = std::abs(v2);
      v3 = std::abs(v3);
      return *this;
    }

    void saturate () {
      v1 = std::min (std::max (v1, (vtype)0), (vtype)0xffff);
      v2 = std::min (std::max (v2, (vtype)0), (vtype)0xffff);
      v3 = std::min (std::max (v3, (vtype)0), (vtype)0xffff);
    }
    
    accu& operator*= (vtype f) {
      v1 *= f;
      v2 *= f;
      v3 *= f;
      return *this;
    }

    accu operator* (vtype f) const {
      accu a = *this;
      return a *= f;
    }
    
    accu& operator+= (vtype f) {
      v1 += f;
      v2 += f;
      v3 += f;
      return *this;
    }

    accu operator+ (vtype f) const {
      accu a = *this;
      return a += f;
    }
    
    accu& operator/= (vtype f) {
      v1 /= f;
      v2 /= f;
      v3 /= f;
      return *this;
    }
    
    accu operator/ (vtype f) const {
      accu a = *this;
      a /= f;
      return a;
    }
    
    accu& operator+= (const accu& other) {
      v1 += other.v1;
      v2 += other.v2;
      v3 += other.v3;
      return *this;
    }
    
    accu operator+ (const accu& other) const {
      accu a = *this;
      a += other;
      return a;
    }

    accu& operator-= (const accu& other) {
      v1 -= other.v1;
      v2 -= other.v2;
      v3 -= other.v3;
      return *this;
    }
    
    accu& operator= (const Image::iterator& background) {
      double r = 0, g = 0, b = 0;
      background.getRGB(r, g, b);
      v1 = (vtype)(r * 0xffff);
      v2 = (vtype)(g * 0xffff);
      v3 = (vtype)(b * 0xffff);
      return *this;
    }
    
    void getRGB (vtype& r, vtype& g, vtype& b) {
      r = v1; g = v2; b = v3;
    }
    
    void getL (vtype& l) {
      l = (11 * v1 + 16 * v2 + 5 * v3) / 32;
    }
    
    void setRGB (vtype r, vtype g, vtype b) {
      v1 = r; v2 = g; v3 = b;
    }
  };
  
  rgb16_iterator (Image& _image)
    : ptr_begin((uint16_t*)_image.getRawData()), image (_image),
      stride(_image.stride()) {
    ptr = ptr_begin;
  }
  
  rgb16_iterator& at (int x, int y) {
    ptr = ptr_begin + y * stride / 2 + x * 3;
    return *this;
  }
    
  rgb16_iterator& operator++ () {
    ptr += 3;
    return *this;
  }

  rgb16_iterator& operator-- () {
    ptr -= 3;
    return *this;
  }
    
  accu operator* () {
    accu a;
    a.v1 = ptr[0];
    a.v2 = ptr[1];
    a.v3 = ptr[2];
    return a;
  }
    
  rgb16_iterator& set (const accu& a) {
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
    typedef int32_t vtype;
    vtype v1;
    
    accu () { v1 = 0; }
    
    static accu one () {
      accu a;
      a.v1 = 0xff;
      return a;
    }

    accu& abs() {
      v1 = std::abs(v1);
      return *this;
    }

    void saturate () {
      v1 = std::min (std::max (v1, (vtype)0), (vtype)0xff);
    }

    accu& operator*= (vtype f) {
      v1 *= f;
      return *this;
    }

    accu operator* (vtype f) const {
      accu a = *this;
      return a *= f;
    }

    accu& operator+= (vtype f) {
      v1 += f;
      return *this;
    }

    accu operator+ (vtype f) const {
      accu a = *this;
      return a += f;
    }
      
    accu& operator/= (vtype f) {
      v1 /= f;
      return *this;
    }

    accu operator/ (vtype f) const {
      accu a = *this;
      a /= f;
      return a;
    }
    
    accu& operator+= (const accu& other) {
      v1 += other.v1;
      return *this;
    }

    accu operator+ (const accu& other) const {
      accu a = *this;
      a += other;
      return a;
    }
    
    accu& operator-= (const accu& other) {
      v1 -= other.v1;
      return *this;
    }
      
    accu& operator= (const Image::iterator& background) {
      v1 = background.getL();
      return *this;
    }
    
    void getRGB (vtype& r, vtype& g, vtype& b) {
      r = g = b = v1;
    }
    
    void getL (vtype& l) {
      l = v1;
    }
    
    void setRGB (vtype& r, vtype& g, vtype& b) {
      v1 = (11 * r + 16 * g + 5 * b) / 32;
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

  gray_iterator& operator-- () {
    ptr -= 1;
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

class gray16_iterator
{
public:
  uint16_t* ptr;
  uint16_t* ptr_begin;
  const Image& image;
  const int stride;
  
  class accu
  {
  public:
    typedef int64_t vtype;
    vtype v1;
    
    accu () { v1 = 0; }
    
    static accu one () {
      accu a;
      a.v1 = 0xffff;
      return a;
    }
    
    accu& abs() {
      v1 = std::abs(v1);
      return *this;
    }

    void saturate () {
      v1 = std::min (std::max (v1, (vtype)0), (vtype)0xffff);
    }

    accu& operator*= (vtype f) {
      v1 *= f;
      return *this;
    }

    accu operator* (vtype f) const {
      accu a = *this;
      return a *= f;
    }
    
    accu& operator+= (vtype f) {
      v1 += f;
      return *this;
    }

    accu operator+ (vtype f) const {
      accu a = *this;
      return a += f;
    }
    
    accu& operator/= (vtype f) {
      v1 /= f;
      return *this;
    }
    
    accu operator/ (vtype f) const {
      accu a = *this;
      a /= f;
      return a;
    }
    
    accu& operator+= (const accu& other) {
      v1 += other.v1;
      return *this;
    }
    
    accu operator+ (const accu& other) const {
      accu a = *this;
      a += other;
      return a;
    }
    
    accu& operator-= (const accu& other) {
      v1 -= other.v1;
      return *this;
    }
    
    accu& operator= (const Image::iterator& background) {
      v1 = background.getL();
      return *this;
    }
    
    void getRGB (vtype& r, vtype& g, vtype& b) {
      r = g = b = v1;
    }
    
    void getL (vtype& l) {
      l = v1;
    }
    
    void setRGB (vtype& r, vtype& g, vtype& b) {
      v1 = (11 * r + 16 * g + 5 * b) / 32;
    }
    
  };
     
  gray16_iterator (Image& _image)
    : ptr_begin((uint16_t*)_image.getRawData()), image (_image),
      stride(_image.stride()) {
    ptr = ptr_begin;
  }
    
  gray16_iterator& at (int x, int y) {
    ptr = ptr_begin + y * stride/2 + x;
    return *this;
  }
    
  gray16_iterator& operator++ () {
    ptr += 1;
    return *this;
  }

  gray16_iterator& operator-- () {
    ptr -= 1;
    return *this;
  }
    
  accu operator* () {
    accu a;
    a.v1 = ptr[0];
    return a;
  }
    
  gray16_iterator& set (const accu& a) {
    ptr[0] = a.v1;
    return *this;
  }
};

template <unsigned int bitdepth>
class bit_iterator
{
public:
  uint8_t* ptr;
  uint8_t* ptr_begin;
  int _x;
  const Image& image;
  const int width, stride;
  int bitpos;
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

  // untested, TODO: test !!!
  bit_iterator& operator-- () {
    --_x;
    bitpos += bitdepth;
    if (bitpos < 0 || _x < 0) {
      if (_x < 0)
	_x = width;
      --ptr;
      bitpos = 7;
    }
    return *this;
  }
  
  accu operator* () {
    accu a;
    a.v1 = ((*ptr >> (bitpos - (bitdepth - 1))) & mask) * 0xff / mask;
    return a;
  }
  
  bit_iterator& set (const accu& a) {
    *ptr &= ~(mask << (bitpos - (bitdepth - 1)));
    *ptr |= (a.v1 >> (8 - bitdepth)) << (bitpos - (bitdepth - 1));
    
    return *this;
  }
};


template <template <typename T> class ALGO, class T1>
void codegen (T1& a1)
{
  if (a1.spp == 3) {
    if (a1.bps == 8) {
      ALGO <rgb_iterator> a;
      a (a1);
    } else {
      ALGO <rgb16_iterator> a;
      a (a1);
    }
  }
  else if (a1.spp == 4 && a1.bps == 8) {
    ALGO <rgba_iterator> a;
    a (a1);
  }
  else if (a1.bps == 16) {
    ALGO <gray16_iterator> a;
    a(a1);
  }
  else if (a1.bps == 8) {
    ALGO <gray_iterator> a;
    a(a1);
  }
  else if (a1.bps == 4) {
    ALGO <bit_iterator<4> > a;
    a (a1);
  }
  else if (a1.bps == 2) {
    ALGO <bit_iterator<2> > a;
    a (a1);
  }
  else if (a1.bps == 1) {
    ALGO <bit_iterator<1> > a;
    a (a1);
  }
}

template <template <typename T> class ALGO, class T1, class T2>
void codegen (T1& a1, T2& a2)
{
  if (a1.spp == 3) {
    if (a1.bps == 8) {
      ALGO <rgb_iterator> a;
      a (a1, a2);
    } else {
      ALGO <rgb16_iterator> a;
      a (a1, a2);
    }
  }
  else if (a1.bps == 16) {
    ALGO <gray16_iterator> a;
    a(a1, a2);
  }
  else if (a1.bps == 8) {
    ALGO <gray_iterator> a;
    a(a1, a2);
  }
  else if (a1.bps == 4) {
    ALGO <bit_iterator<4> > a;
    a (a1, a2);
  }
  else if (a1.bps == 2) {
    ALGO <bit_iterator<2> > a;
    a (a1, a2);
  }
  else if (a1.bps == 1) {
    ALGO <bit_iterator<1> > a;
    a (a1, a2);
  }
}

template <template <typename T> class ALGO, class T1, class T2, class T3>
void codegen (T1& a1, T2& a2, T3& a3)
{
  if (a1.spp == 3) {
    if (a1.bps == 8) {
      ALGO <rgb_iterator> a;
      a (a1, a2, a3);
    } else {
      ALGO <rgb16_iterator> a;
      a (a1, a2, a3);
    }
  }
  else if (a1.spp == 4 && a1.bps == 8) {
    ALGO <rgba_iterator> a;
    a (a1, a2, a3);
  }
  else if (a1.bps == 16) {
    ALGO <gray16_iterator> a;
    a(a1, a2, a3);
  }
  else if (a1.bps == 8) {
    ALGO <gray_iterator> a;
    a(a1, a2, a3);
  }
  else if (a1.bps == 4) {
    ALGO <bit_iterator<4> > a;
    a (a1, a2, a3);
  }
  else if (a1.bps == 2) {
    ALGO <bit_iterator<2> > a;
    a (a1, a2, a3);
  }
  else if (a1.bps == 1) {
    ALGO <bit_iterator<1> > a;
    a (a1, a2, a3);
  }
}

template <template <typename T> class ALGO, class T1, class T2, class T3, class T4>
void codegen (T1& a1, T2& a2, T3& a3, T4& a4)
{
  if (a1.spp == 3) {
    if (a1.bps == 8) {
      ALGO <rgb_iterator> a;
      a (a1, a2, a3, a4);
    } else {
      ALGO <rgb16_iterator> a;
      a (a1, a2, a3, a4);
    }
  }
  else if (a1.spp == 4 && a1.bps == 8) {
    ALGO <rgba_iterator> a;
    a (a1, a2, a3, a4);
  }
  else if (a1.bps == 16) {
    ALGO <gray16_iterator> a;
    a(a1, a2, a3, a4);
  }
  else if (a1.bps == 8) {
    ALGO <gray_iterator> a;
    a(a1, a2, a3, a4);
  }
  else if (a1.bps == 4) {
    ALGO <bit_iterator<4> > a;
    a (a1, a2, a3, a4);
  }
  else if (a1.bps == 2) {
    ALGO <bit_iterator<2> > a;
    a (a1, a2, a3, a4);
  }
  else if (a1.bps == 1) {
    ALGO <bit_iterator<1> > a;
    a (a1, a2, a3, a4);
  }
}

template <template <typename T> class ALGO, class T1, class T2, class T3, class T4, class T5>
void codegen (T1& a1, T2& a2, T3& a3, T4& a4, T5& a5)
{
  if (a1.spp == 3) {
    if (a1.bps == 8) {
      ALGO <rgb_iterator> a;
      a (a1, a2, a3, a4, a5);
    } else {
      ALGO <rgb16_iterator> a;
      a (a1, a2, a3, a4, a5);
    }
  }
  else if (a1.spp == 4 && a1.bps == 8) {
    ALGO <rgba_iterator> a;
    a (a1, a2, a3, a4, a5);
  }
  else if (a1.bps == 16) {
    ALGO <gray16_iterator> a;
    a(a1, a2, a3, a4, a5);
  }
  else if (a1.bps == 8) {
    ALGO <gray_iterator> a;
    a(a1, a2, a3, a4, a5);
  }
  else if (a1.bps == 4) {
    ALGO <bit_iterator<4> > a;
    a (a1, a2, a3, a4, a5);
  }
  else if (a1.bps == 2) {
    ALGO <bit_iterator<2> > a;
    a (a1, a2, a3, a4, a5);
  }
  else if (a1.bps == 1) {
    ALGO <bit_iterator<1> > a;
    a (a1, a2, a3, a4, a5);
  }
}

template <template <typename T> class ALGO, class T1, class T2, class T3, class T4, class T5,  class T6>
void codegen (T1& a1, T2& a2, T3& a3, T4& a4, T5& a5, T6& a6)
{
  if (a1.spp == 3) {
    if (a1.spp == 8) {
      ALGO <rgb_iterator> a;
      a (a1, a2, a3, a4, a5, a6);
    } else {
      ALGO <rgb16_iterator> a;
      a (a1, a2, a3, a5, a5, a6);
    }
  }
  else if (a1.spp == 4 && a1.bps == 8) {
    ALGO <rgba_iterator> a;
    a (a1, a2, a3, a5, a5, a6);
  }
  else if (a1.bps == 16) {
    ALGO <gray16_iterator> a;
    a(a1, a2, a3, a4, a5, a6);
  }
  else if (a1.bps == 8) {
    ALGO <gray_iterator> a;
    a(a1, a2, a3, a4, a5, a6);
  }
  else if (a1.bps == 4) {
    ALGO <bit_iterator<4> > a;
    a (a1, a2, a3, a4, a5, a6);
  }
  else if (a1.bps == 2) {
    ALGO <bit_iterator<2> > a;
    a (a1, a2, a3, a4, a5, a6);
  }
  else if (a1.bps == 1) {
    ALGO <bit_iterator<1> > a;
    a (a1, a2, a3, a4, a5, a6);
  }
}

template <template <typename T> class ALGO,
	  class T1, class T2, class T3, class T4,
	  class T5, class T6, class T7>
void codegen (T1& a1, T2& a2, T3& a3, T4& a4,
	      T5& a5, T6& a6, T7& a7)
{
  if (a1.spp == 3) {
    if (a1.bps == 8) {
      ALGO <rgb_iterator> a;
      a (a1, a2, a3, a4, a5, a6, a7);
    } else {
      ALGO <rgb16_iterator> a;
      a (a1, a2, a3, a4, a5, a6, a7);
    }
  }
  else if (a1.spp == 4 && a1.bps == 8) {
    ALGO <rgba_iterator> a;
    a (a1, a2, a3, a5, a5, a6, a7);
  }
  else if (a1.bps == 16) {
    ALGO <gray16_iterator> a;
    a (a1, a2, a3, a4, a5, a6, a7);
  }
  else if (a1.bps == 8) {
    ALGO <gray_iterator> a;
    a (a1, a2, a3, a4, a5, a6, a7);
  }
  else if (a1.bps == 4) {
    ALGO <bit_iterator<4> > a;
    a (a1, a2, a3, a4, a5, a6, a7);
  }
  else if (a1.bps == 2) {
    ALGO <bit_iterator<2> > a;
    a (a1, a2, a3, a4, a5, a6, a7);
  }
  else if (a1.bps == 1) {
    ALGO <bit_iterator<1> > a;
    a (a1, a2, a3, a4, a5, a6, a7);
  }
}

// with return

template <class T0, template <typename T> class ALGO,
	  class T1, class T2>
T0 codegen_return (T1& a1, T2& a2)
{
  if (a1.spp == 3) {
    if (a1.bps == 8) {
      ALGO <rgb_iterator> a;
      return a (a1, a2);
    } else {
      ALGO <rgb16_iterator> a;
      return a (a1, a2);
    }
  }
  else if (a1.spp == 4 && a1.bps == 8) {
    ALGO <rgba_iterator> a;
    return a (a1, a2);
  }
  else if (a1.bps == 16) {
    ALGO <gray16_iterator> a;
    return a (a1, a2);
  }
  else if (a1.bps == 8) {
    ALGO <gray_iterator> a;
    return a (a1, a2);
  }
  else if (a1.bps == 4) {
    ALGO <bit_iterator<4> > a;
    return a (a1, a2);
  }
  else if (a1.bps == 2) {
    ALGO <bit_iterator<2> > a;
    return a (a1, a2);
  }
  else if (a1.bps == 1) {
    ALGO <bit_iterator<1> > a;
    return a (a1, a2);
  }
  
  // warn unhandled
  T0 t;
  return t;
}

template <class T0, template <typename T> class ALGO,
	  class T1, class T2, class T3>
T0 codegen_return (T1& a1, T2& a2, T3& a3)
{
  if (a1.spp == 3) {
    if (a1.bps == 8) {
      ALGO <rgb_iterator> a;
      return a (a1, a2, a3);
    } else {
      ALGO <rgb16_iterator> a;
      return a (a1, a2, a3);
    }
  }
  else if (a1.spp == 4 && a1.bps == 8) {
    ALGO <rgba_iterator> a;
    return a (a1, a2, a3);
  }
  else if (a1.bps == 16) {
    ALGO <gray16_iterator> a;
    return a (a1, a2, a3);
  }
  else if (a1.bps == 8) {
    ALGO <gray_iterator> a;
    return a (a1, a2, a3);
  }
  else if (a1.bps == 4) {
    ALGO <bit_iterator<4> > a;
    return a (a1, a2, a3);
  }
  else if (a1.bps == 2) {
    ALGO <bit_iterator<2> > a;
    return a (a1, a2, a3);
  }
  else if (a1.bps == 1) {
    ALGO <bit_iterator<1> > a;
    return a (a1, a2, a3);
  }
  
  // warn unhandled
  T0 t;
  return t;
}

template <class T0, template <typename T> class ALGO,
	  class T1, class T2, class T3, class T4,
	  class T5, class T6, class T7>
T0 codegen_return (T1& a1, T2& a2, T3& a3, T4& a4,
		   T5& a5, T6& a6, T7& a7)
{
  if (a1.spp == 3) {
    if (a1.bps == 8) {
      ALGO <rgb_iterator> a;
      return a (a1, a2, a3, a4, a5, a6, a7);
    } else {
      ALGO <rgb16_iterator> a;
      return a (a1, a2, a3, a4, a5, a6, a7);
    }
  }
  else if (a1.spp == 4 && a1.bps == 8) {
    ALGO <rgba_iterator> a;
    return a (a1, a2, a3, a5, a5, a6, a7);
  }
  else if (a1.bps == 16) {
    ALGO <gray16_iterator> a;
    return a (a1, a2, a3, a4, a5, a6, a7);
  }
  else if (a1.bps == 8) {
    ALGO <gray_iterator> a;
    return a (a1, a2, a3, a4, a5, a6, a7);
  }
  else if (a1.bps == 4) {
    ALGO <bit_iterator<4> > a;
    return a (a1, a2, a3, a4, a5, a6, a7);
  }
  else if (a1.bps == 2) {
    ALGO <bit_iterator<2> > a;
    return a (a1, a2, a3, a4, a5, a6, a7);
  }
  else if (a1.bps == 1) {
    ALGO <bit_iterator<1> > a;
    return a (a1, a2, a3, a4, a5, a6, a7);
  }
  
  // warn unhandled
  T0 t;
  return t;
}
