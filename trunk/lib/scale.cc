/*
 * Copyright (C) 2006 - 2014 René Rebe, ExactCODE GmbH Germany.
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

#include <string.h> // memset

#include <cmath>
#include <iostream>
#include <algorithm>

#include "Image.hh"
#include "ImageIterator2.hh"
#include "Codecs.hh"

#include "Colorspace.hh"

#include "scale.hh"

void scale (Image& image, double scalex, double scaley)
{
  if (scalex == 1.0 && scaley == 1.0)
    return;
  
  // thru the codec?
  if (!image.isModified() && image.getCodec())
    if (image.getCodec()->scale(image, scalex, scaley))
      return;
  
  if (scalex <= 0.5)
    box_scale (image, scalex, scaley);
  else
    bilinear_scale (image, scalex, scaley);
}

template <typename T>
struct nearest_scale_template
{
  void operator() (Image& new_image, double scalex, double scaley)
  {
    
    Image image;
    image.copyTransferOwnership (new_image);
    
    new_image.resize ((int)(scalex * (double) image.w),
		      (int)(scaley * (double) image.h));
    new_image.setResolution (scalex * image.resolutionX(),
			     scaley * image.resolutionY());
    
    #pragma omp parallel for schedule (dynamic, 16)
    for (int y = 0; y < new_image.h; ++y)
    {
      T dst (new_image);
      dst.at(0, y);

      T src (image);
      for (int x = 0; x < new_image.w; ++x) {
	const int bx = (int) (((double) x) / scalex);
	const int by = (int) (((double) y) / scaley);
	
	typename T::accu a;
	a  = *src.at (bx, by);
	dst.set (a);
	++dst;
      }
    }
  }
};

void nearest_scale (Image& image, double scalex, double scaley)
{
  if (scalex == 1.0 && scaley == 1.0)
    return;
  codegen<nearest_scale_template> (image, scalex, scaley);
}


template <typename T>
struct bilinear_scale_template
{
  void operator() (Image& new_image, double scalex, double scaley, bool fixed)
  {
    if (!fixed) {
      scalex = (int)(scalex * new_image.w);
      scaley = (int)(scaley * new_image.h);
    }
      
    Image image;
    image.copyTransferOwnership (new_image);

    new_image.resize (scalex, scaley);
    new_image.setResolution (new_image.w * image.resolutionX() / image.w,
			     new_image.h * image.resolutionY() / image.h);
    
    // cache x offsets, 2x speedup
    float bxmap[new_image.w];
    int sxmap[new_image.w];
    int sxxmap[new_image.w];
    for (int x = 0; x < new_image.w; ++x) {
      bxmap[x] = (float)x / (new_image.w - 1) * (image.w - 1);
      sxmap[x] = (int)floor(bxmap[x]);
      sxxmap[x] = sxmap[x] == (image.w - 1) ? sxmap[x] : sxmap[x] + 1;
    }
    
    #pragma omp parallel for schedule (dynamic, 16)
    for (int y = 0; y < new_image.h; ++y)
    {
      T dst (new_image);
      dst.at(0, y);

      const float by = (float)y / (new_image.h - 1) * (image.h - 1);
      
      const int sy = (int)floor(by);
      const int ydist = (int) ((by - sy) * 256);
      const int syy = sy == (image.h - 1) ? sy : sy + 1;

      T src (image);
      for (int x = 0; x < new_image.w; ++x) {
	const float bx = bxmap[x];
	const int sx = sxmap[x];;
	const int xdist = (int) ((bx - sx) * 256);
	const int sxx = sxxmap[x];;

	typename T::accu a1, a2;
	a1  = (*src.at (sx,  sy )) * ((256-xdist));
	a1 += (*src.at (sxx, sy )) * (xdist      );
	a1 /= 256;
	
	a2  = (*src.at (sx,  syy)) * ((256-xdist));
	a2 += (*src.at (sxx, syy)) * (xdist      );
	a2 /= 256;
	
	a1 = a1 * (256-ydist) + a2 * ydist;
	a1 /= 256;
	
	dst.set(a1);
	++dst;
      }
    }
  }
};

void bilinear_scale (Image& image, double scalex, double scaley, bool fixed)
{
  if (scalex == 1.0 && scaley == 1.0)
    return;
  codegen<bilinear_scale_template> (image, scalex, scaley, fixed);
}

template <typename T>
struct box_scale_template
{
  void operator() (Image& new_image, double scalex, double scaley)
  {
    Image image;
    image.copyTransferOwnership (new_image);
    
    new_image.resize ((int)(scalex * (double) image.w),
		      (int)(scaley * (double) image.h));
    new_image.setResolution (scalex * image.resolutionX(),
			     scaley * image.resolutionY());
    
    T src (image);
    T dst (new_image);
  
    // prepare boxes
#if defined(_MSC_VER) || defined(__clang__)
    std::vector<typename T::accu> boxes(new_image.w);
#else
    typename T::accu boxes [new_image.w]; 
#endif

#if defined(_MSC_VER)
    std::vector<int> count(new_image.w);
    std::vector<int> bindex(image.w); // pre-computed box-indexes
#else
    int count [new_image.w];
    int bindex [image.w]; // pre-computed box indexes
#endif
    for (int sx = 0; sx < image.w; ++sx)
      bindex[sx] = std::min ((int)(scalex * sx), new_image.w - 1);
    
    int dy = 0;
    for (int sy = 0; dy < new_image.h && sy < image.h; ++dy)
      {
	// clear for accumulation
	for (int x = 0; x < new_image.w; ++x) {
	  boxes[x] = typename T::accu();
	  count[x] = 0;
	}
	
	for (; sy < image.h && scaley * sy < dy + 1; ++sy) {
	  //      std::cout << "sy: " << sy << " from " << image.h << std::endl;
	  for (int sx = 0; sx < image.w; ++sx) {
	    //	std::cout << "sx: " << sx << " -> " << dx << std::endl;
	    const int dx = bindex[sx];
	    boxes[dx] += *src; ++src;
	    ++count[dx];
	  }
	}
	
	// set box
	//    std::cout << "dy: " << dy << " from " << new_image.h << std::endl;
	for (int dx = 0; dx < new_image.w; ++dx) {
	  //      std::cout << "setting: dx: " << dx << ", from: " << new_image.w
	  //       		<< ", count: " << count[dx] << std::endl;      
	  boxes[dx] /= count[dx];
	  dst.set (boxes[dx]);
	  ++dst;
	}
      }
  }
};

void box_scale (Image& image, double scalex, double scaley)
{
  if (scalex == 1.0 && scaley == 1.0)
    return;
  codegen<box_scale_template> (image, scalex, scaley);
}

inline Image::iterator CubicConvolution (int distance,
					 const Image::iterator& f0,
					 const Image::iterator& f1,
					 const Image::iterator& f2,
					 const Image::iterator& f3) 
{
  Image::iterator it = f0;
  it = ( /*(    f1 + f3 - f0   - f2 ) * distance * distance * distance
	   + (f0*2 + f2 - f1*2 - f3 ) * distance * distance
	   +*/ (  f2 - f1             ) * distance ) / (256)
    + f1;
  return it;
}

/* 0 0 0 0
   0 4 0 0
   0 0 -13.5 6
   0 0 6.1 -2.45 */

void bicubic_scale (Image& new_image, double scalex, double scaley)
{
  if (scalex == 1.0 && scaley == 1.0)
    return;

  Image image;
  image.copyTransferOwnership (new_image);
  
  new_image.resize ((int)(scalex * (double) image.w),
		    (int)(scaley * (double) image.h));
  new_image.setResolution (scalex * image.resolutionX(),
			   scaley * image.resolutionY());
  
  Image::iterator dst = new_image.begin();
  Image::iterator src = image.begin();

  Image::iterator r0 = image.begin();
  Image::iterator r1 = image.begin();
  Image::iterator r2 = image.begin();
  Image::iterator r3 = image.begin();

  for (int y = 0; y < new_image.h; ++y) {
    double by = .5+y / scaley;
    const int sy = std::min((int)by, image.h-1);
    const int ydist = (int) ((by - sy) * 256);
    
    const int sy0 = std::max(sy-1, 0);
    const int sy2 = std::min(sy+1, image.h-1);
    const int sy3 = std::min(sy+2, image.h-1);
    
    for (int x = 0; x < new_image.w; ++x) {
      
      const double bx = .5+x / scalex;
      const int sx = std::min((int)bx, image.w - 1);
      const int xdist = (int) ((bx - sx) * 256);
      
      const int sx0 = std::max(sx-1, 0);
      const int sx2 = std::min(sx+1, image.w-1);
      const int sx3 = std::min(sx+2, image.w-1);
      
      //      xdist = ydist = 0;
      r0 = CubicConvolution (xdist,
			     *src.at(sx0,sy0), *src.at(sx,sy0),
			     *src.at(sx2,sy0), *src.at(sx3,sy0));
      r1 = CubicConvolution (xdist,
			     *src.at(sx0,sy),  *src.at(sx,sy),
			     *src.at(sx2,sy),  *src.at(sx3,sy));
      r2 = CubicConvolution (xdist,
			     *src.at(sx0,sy2), *src.at(sx,sy2),
			     *src.at(sx2,sy2), *src.at(sx3,sy2));
      r3 = CubicConvolution (xdist,
			     *src.at(sx0,sy3), *src.at(sx,sy3),
			     *src.at(sx2,sy3), *src.at(sx3,sy3));
      
      dst.set (CubicConvolution (ydist, r0, r1, r2, r3));
      ++dst;
    }
  }
}
#ifndef _MSC_VER

template <typename T>
T interp(float x, float y, const T&a,  const T& b, const T& c, const T& d)
{
  // x, y: [0 .. 1] // we use 256-scale for fix-point int images
  if (x >= y) {
    float b1 = -(x - 1);
    float b2 = (x - 1) - (y - 1);
    float b3 = 1 - b1 - b2;
    return (a * (256 * b1) + d * (256 * b2) + c * (256 * b3)) / 256;
  } else {
    float b1 = -(y - 1);
    float b2 = -((x - 1) - (y - 1));
    float b3 = 1 - b1 - b2;
    return (a * (256 * b1) + b * (256 * b2) + c * (256 * b3)) / 256;
  }
}

template <typename T>
struct ddt_scale_template
{
  void operator() (Image& new_image, double scalex, double scaley, bool extended)
  {
    Image image;
    image.copyTransferOwnership (new_image);
    
    new_image.resize((int)(scalex * (double) image.w),
		     (int)(scaley * (double) image.h));
    new_image.setResolution (scalex * image.resolutionX(),
			     scaley * image.resolutionY());
    
    // first scan the source image and build a direction map
    // TODO: we could do the check on-the-fly, ...
    char dir_map [image.h - 1][image.w - 1];
    
    // A - D
    // |   |
    // B - C
    T src_a(image), src_b(image), src_c(image), src_d(image);
    for (int y = 0; y < image.h-1; ++y) {
      src_a.at(0, y);
      src_b.at(0, y+1);
      src_c.at(1, y+1);
      src_d.at(1, y);
      
      for (int x = 0; x < image.w-1; ++x) {
	typename T::accu::vtype a, b, c, d;
	(*src_a).getL(a); ++src_a;
	(*src_b).getL(b); ++src_b;
	(*src_c).getL(c); ++src_c;
	(*src_d).getL(d); ++src_d;
	
	//std::cout << "x: " << x << ", y: " << y << std::endl;
	//std::cout << "a: " << a << ", b: " << b
	//	  << ", c: " << c << ", d: " << d << std::endl;
	
	if (abs(a-c) < abs(b-d))
	  dir_map[y][x] = '\\';
	else
	  dir_map[y][x] = '/';
	}
    }
    
    if (extended)
      {
	char dir_map2 [image.h - 1][image.w - 1];
	
	for (int y = 1; y < image.h-2; ++y) {
	  for (int x = 1; x < image.w-2; ++x)
	    {
	      uint8_t n1 = 0, n2 = 0;
	      for (int i = 0; i < 3; ++i) {
		for (int j = 0; j < 3; ++j) {
		  n1 += dir_map[y+i][x+j] == '/';
		  n2 += dir_map[y+i][x+j] == '\\';
		}
	      }
	      if (n1 >= 6)
		dir_map2[y][x] = '/';
	      else if (n2 >= 6)
		dir_map2[y][x] = '\\';
	      else 
		dir_map2[y][x] = dir_map[y][x];
	    }
	}

	for (int y = 1; y < image.h-2; ++y)
	  for (int x = 1; x < image.w-2; ++x)
	    dir_map[y][x] = dir_map2[y][x];
      }
    
    if (false)
      {
	int n = 0;
	for (int y = 0; y < image.h-1; ++y) {
	  for (int x = 0; x < image.w-1; ++x) {
	    std::cout << (dir_map[y][x] == '/' ? ' ' : '\\');
	    if (dir_map[y][x] == '/') ++n;
	  }
	  std::cout << std::endl;
	}
	std::cout << "NW-SE: " << n << std::endl;
	
	std::cout << std::endl;
	
	n = 0;
	for (int y = 0; y < image.h-1; ++y) {
	  for (int x = 0; x < image.w-1; ++x) {
	    std::cout << (dir_map[y][x] == '/' ? '/' : ' ');
	    if (dir_map[y][x] != '/') ++n;
	  }
	  std::cout << std::endl;
	}
	std::cout << "NE-SW: " << n << std::endl;
      }
    
    T dst(new_image);
    T src(image);
    for (int y = 0; y < new_image.h; ++y) {
      const float by = (float)y / (new_image.h - 1) * (image.h - 1);
      const int sy = std::min((int)floor(by), image.h - 2);
      const float ydist = by - sy;
      
      for (int x = 0; x < new_image.w; ++x) {
	const float bx = (float)x / (new_image.w - 1) * (image.w - 1);
	const int sx = std::min((int)floor(bx), image.w - 2);
	const float xdist = bx - sx;
	
	if (false)
	  std::cout << "x: " << x << ", y: " << y << " <- "
		    << "bx: " << bx << ", by: " << by
		    << ", sx: " << sx << ", sy: " << sy << " dist: " << xdist <<", " << ydist << std::endl;
	
	typename T::accu v;
	const typename T::accu a = *src.at(sx, sy);
	const typename T::accu b = *src.at(sx, sy + 1);
	const typename T::accu c = *src.at(sx + 1, sy + 1);
	const typename T::accu d = *src.at(sx + 1, sy);
	
	// which triangle does the point fall into?
	if (dir_map[sy][sx] == '\\') {
	  v = interp(xdist, ydist, a, b, c, d);
	}
	else { // '/'
	  // virtually rotate triangles by 90
	  v = interp(ydist, 1. - xdist, d, a, b, c);
	}
	
	dst.set(v);
	++dst;
      }
    }

    // syntetic test
    if (false) {
      dst.at(0, 0);
      for (int y = 0; y < new_image.h; ++y) {
	for (int x = 0; x < new_image.w; ++x) {
	  typename T::accu v, a, b, c, d;
	  a.setRGB(0, 0, 0);
	  b.setRGB(33, 33, 33);
	  c.setRGB(255, 255, 255);
	  d.setRGB(128, 128, 128);
	  
	  v = interp(float(x) / new_image.w, (float)y / new_image.h, a, b, c, d);
	  //v = interp((float)y / new_image.h, 1. - float(x) / new_image.w, d, a, b, c);
	  
	  dst.set(v);
	  ++dst;
	}
      }
    }
  }
};
  
void ddt_scale (Image& image, double scalex, double scaley, bool extended)
{
  if (scalex == 1.0 && scaley == 1.0)
    return;
  codegen<ddt_scale_template> (image, scalex, scaley, extended);
}

#endif

void box_scale_grayX_to_gray8 (Image& new_image, double scalex, double scaley)
{
  if (scalex == 1.0 && scaley == 1.0)
    return;
  
  Image image;
  image.copyTransferOwnership (new_image);
  
  new_image.bps = 8; // always output 8bit gray
  new_image.resize((int)(scalex * (double) image.w),
		   (int)(scaley * (double) image.h));
  new_image.setResolution (scalex * image.resolutionX(),
			   scaley * image.resolutionY());
  uint8_t* src = image.getRawData();
  uint8_t* dst = new_image.getRawData();
  
#ifdef _MSC_VER
  std::vector<uint32_t> boxes(new_image.w);
  std::vector<uint32_t> count(new_image.w);
  std::vector<int> bindex(image.w);
#else
  uint32_t boxes[new_image.w];
  uint32_t count[new_image.w];
  // pre-compute box indexes
  int bindex [image.w];
#endif
  for (int sx = 0; sx < image.w; ++sx)
    bindex[sx] = std::min ((int)(scalex * sx), new_image.w - 1);
  
  const int bps = image.bps;
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
  
  int dy = 0;
  for (int sy = 0; dy < new_image.h && sy < image.h; ++dy)
    {
      // clear for accumulation
      memset (&boxes[0], 0, sizeof(boxes[0]) * new_image.w);
      memset (&count[0], 0, sizeof(count[0]) * new_image.w);
      
      for (; sy < image.h && sy * scaley < dy + 1; ++sy)
	{
	  uint8_t z = 0;
	  unsigned int bits = 0;
	  
	  for (int sx = 0; sx < image.w; ++sx)
	    {
	      if (bits == 0) {
		z = *src++;
		bits = 8;
	      }
	      
	      const int dx = bindex[sx];
	      boxes[dx] += gray_lookup[z >> bitshift];
	      ++count[dx];
	      
	      z <<= bps;
	      bits -= bps;
	    }
	}
      
      for (int dx = 0; dx < new_image.w; ++dx) {
	*dst = (boxes[dx] / count[dx]);
	++dst;
      }
    }
}

void thumbnail_scale (Image& image, double scalex, double scaley)
{
  // only optimize the regular thumbnail down-scaling
  if (scalex > 1 || scaley > 1)
    return scale(image, scalex, scaley);
  
  // thru the codec?
  if (!image.isModified() && image.getCodec())
    if (image.getCodec()->scale(image, scalex, scaley))
      return;
  
  // quick sub byte scaling
  if (image.bps <= 8 && image.spp == 1) {
    box_scale_grayX_to_gray8(image, scalex, scaley);
  }
  else {
    if (image.spp == 1 && image.bps > 8)
      colorspace_by_name(image, "gray");
    else if (image.spp > 3 || image.bps > 8)
      colorspace_by_name(image, "rgb");
    
    box_scale(image, scalex, scaley);
  }
}
