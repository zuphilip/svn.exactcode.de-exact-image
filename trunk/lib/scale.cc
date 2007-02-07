#include <math.h>

#include <iostream>
#include <algorithm>

#include "Image.hh"
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
  
  if (scalex < 0.5)
    box_scale (image, scalex, scaley);
  else
    bilinear_scale (image, scalex, scaley);
}

void nearest_scale (Image& new_image, double scalex, double scaley)
{
  Image image;
  image.copyTransferOwnership (new_image);
  
  new_image.New ((int)(scalex * (double) image.w),
		 (int)(scaley * (double) image.h));
  new_image.xres = (int) (scalex * image.xres);
  new_image.yres = (int) (scaley * image.yres);

  Image::iterator dst = new_image.begin();
  Image::iterator src = image.begin();
  for (int y=0; y < new_image.h; ++y)
    for (int x=0; x < new_image.w; ++x) {
      const int bx = (int) (((double) x) / scalex);
      const int by = (int) (((double) y) / scaley);
      
      dst.set (*src.at (bx, by) );
      ++dst;
    }
}

void bilinear_scale (Image& new_image, double scalex, double scaley)
{
  Image image;
  image.copyTransferOwnership (new_image);

  new_image.New ((int)(scalex * (double) image.w),
		 (int)(scaley * (double) image.h));
  new_image.xres = (int) (scalex * image.xres);
  new_image.yres = (int) (scaley * image.yres);
  
  uint8_t* data = image.getRawData();
  
  /* handcrafted due popular request */
  if (new_image.spp == 1 && new_image.bps < 8)
    {
      const int bps = image.bps;
      const int spb = 8 / bps; // Samples Per Byte
      const int mask = (1 << bps) - 1;
      const int stride = image.Stride ();
      const int bshift = 8 - bps;
      const int bscale = 255 / mask;
      
      //std::cerr << "bps: " << bps << ", spb: " << spb
      // 	  << ", mask: " << mask << ", stride: " << stride << std::endl;
      
      uint8_t* dst = new_image.getRawData();
      uint8_t v = 0;
      
      for (int y = 0; y < new_image.h; ++y) {
	double by = (-1.0+image.h) * y / new_image.h;
	const int sy = (int)floor(by);
	const int ydist = (int) ((by - sy) * 256);
	const int syy = sy+1;
	
	int x;
	for (x = 0; x < new_image.w;) {
	  const double bx = (-1.0+image.w) * x / new_image.w;
	  const int sx = (int)floor(bx);
	  const int xdist = (int) ((bx - sx) * 256);
	  const int sxx = sx+1;
	  
	  v <<= bps;
	  
	  v |=
	    (( (data[sx/spb  + sy*stride]  >> (bshift - (sx%spb)*bps) ) & mask) * bscale * (256-xdist) * (256-ydist) +
	     ( (data[sxx/spb + sy*stride]  >> (bshift - (sxx%spb)*bps)) & mask) * bscale * xdist       * (256-ydist) +
	     ( (data[sx/spb  + syy*stride] >> (bshift - (sx%spb)*bps) ) & mask) * bscale * (256-xdist) * ydist +
	     ( (data[sxx/spb + syy*stride] >> (bshift - (sxx%spb)*bps)) & mask) * bscale * xdist       * ydist)
	    >> (16 + bshift);
	  
	  ++x;
	  if (x % spb == 0)
	    *dst++ = v;
	}
	int remainder = spb - x % spb;
	if (remainder != spb) {
	  v <<= bps * remainder;
	  *dst++ = v;
	}
      }
      return;
    }
  
  /* handcrafted due popular request */
  if (new_image.spp == 1 && new_image.bps == 8)
    {
      const int stride = image.Stride ();
      uint8_t* dst = new_image.getRawData();
      
      for (int y = 0; y < new_image.h; ++y) {
	const double by = (-1.0+image.h) * y / new_image.h;
	const int sy = (int)floor(by);
	const int ydist = (int) ((by - sy) * 256);
	const int syy = sy+1;
	
	for (int x = 0; x < new_image.w; ++x) {
	  const double bx = (-1.0+image.w) * x / new_image.w;
	  const int sx = (int)floor(bx);
	  const int xdist = (int) ((bx - sx) * 256);
	  const int sxx = sx+1;
	  
	  *dst = (data [sx  + sy*stride]  * (256-xdist) * (256-ydist) +
		  data [sxx + sy*stride]  * xdist       * (256-ydist) +
		  data [sx  + syy*stride] * (256-xdist) * ydist +
		  data [sxx + syy*stride] * xdist       * ydist)
	    >> 16;
	  ++dst;
	}
      }
      return;
    }
  
   /* handcrafted due popular request */
  if (new_image.spp == 3 && new_image.bps == 8)
    {
      const int stride = image.Stride ();
      uint8_t* dst = new_image.getRawData();
      
      for (int y = 0; y < new_image.h; ++y) {
	const double by = (-1.0+image.h) * y / new_image.h;
	const int sy = (int)floor(by);
	const int ydist = (int) ((by - sy) * 256);
	const int syy = sy+1;
	
	for (int x = 0; x < new_image.w; ++x) {
	  const double bx = (-1.0+image.w) * x / new_image.w;
	  const int sx = 3*(int)floor(bx);
	  const int xdist = (int) ((bx - sx/3) * 256);
	  const int sxx = sx+3;
	  *dst++ = (data [sx  +0+ sy*stride]  * (256-xdist) * (256-ydist) +
		    data [sxx +0+ sy*stride]  * xdist       * (256-ydist) +
		    data [sx  +0+ syy*stride] * (256-xdist) * ydist +
		    data [sxx +0+ syy*stride] * xdist       * ydist) >> 16;
	  *dst++ = (data [sx  +1+ sy*stride]  * (256-xdist) * (256-ydist) +
		    data [sxx +1+ sy*stride]  * xdist       * (256-ydist) +
		    data [sx  +1+ syy*stride] * (256-xdist) * ydist +
		    data [sxx +1+ syy*stride] * xdist       * ydist) >> 16;
	  *dst++ = (data [sx  +2+ sy*stride]  * (256-xdist) * (256-ydist) +
		    data [sxx +2+ sy*stride]  * xdist       * (256-ydist) +
		    data [sx  +2+ syy*stride] * (256-xdist) * ydist +
		    data [sxx +2+ syy*stride] * xdist       * ydist) >> 16;
	}
      }
      return;
    }
  
  Image::iterator dst = new_image.begin();
  Image::iterator src = image.begin();
      
  for (int y = 0; y < new_image.h; ++y) {
    const double by = (-1.0+image.h) * y / new_image.h;
    const int sy = (int)floor(by);
    const int ydist = (int) ((by - sy) * 256);
    const int syy = sy+1;

    for (int x = 0; x < new_image.w; ++x) {
      const double bx = (-1.0+image.w) * x / new_image.w;
      const int sx = (int)floor(bx);
      const int xdist = (int) ((bx - sx) * 256);
      const int sxx = sx+1;

      if (false && x < 8 && y < 8) {
	std::cout << "sx: " << sx << ", sy: " << sy
		  << ", sxx: " << sxx << ", syy: " << syy << std::endl;
	std::cout << "xdist: " << xdist << ", ydist: " << ydist << std::endl;
      }

      dst.set ( (*src.at (sx,  sy ) * (256-xdist) * (256-ydist) +
		 *src.at (sxx, sy ) * xdist       * (256-ydist) +
		 *src.at (sx,  syy) * (256-xdist) * ydist +
		 *src.at (sxx, syy) * xdist       * ydist) /
		(256 * 256) );
      ++dst;
    }
  }
}

void box_scale (Image& new_image, double scalex, double scaley)
{
  Image image;
  image.copyTransferOwnership (new_image);
  
  new_image.New ((int)(scalex * (double) image.w),
		 (int)(scaley * (double) image.h));
  new_image.xres = (int) (scalex * image.xres);
  new_image.yres = (int) (scaley * image.yres);
  
  /* handcrafted due popular request */
  if (new_image.spp == 1 && new_image.bps == 8)
    {
      uint8_t* src = image.getRawData();
      uint8_t* dst = new_image.getRawData();
      
      uint32_t boxes[new_image.w];
      uint32_t count[new_image.w];

      int dy = 0;
      for (int sy = 0; dy < new_image.h && sy < image.h; ++dy) {
	
	// clear for accumulation
	memset (boxes, 0, sizeof (boxes));
	memset (count, 0, sizeof (count));
	
	for (; sy < image.h && sy * scaley < dy + 1; ++sy) {
	  for (int sx = 0; sx < image.w; ++sx) {
	    int dx = std::min ((int)(scalex*sx), new_image.w-1);
	    boxes[dx] += *src; ++src;
	    ++count[dx];
	  }
	}
	
	for (int dx = 0; dx < new_image.w; ++dx) {
	  *dst = (boxes[dx] / count[dx]);
	  ++dst;
	}
      }
      
      return;
    }
  
  Image::iterator src = image.begin();
  Image::iterator dst = new_image.begin();
  
  // prepare boxes
  Image::iterator boxes  [new_image.w];
  int count [new_image.w];
  for (int x = 0; x < new_image.w; ++x)
    boxes[x] = new_image.begin();
  
  int dy = 0;
  for (int sy = 0; dy < new_image.h && sy < image.h; ++dy)
    {
    
      // clear for accumulation
      for (int x = 0; x < new_image.w; ++x) {
	boxes[x].clear();
	count[x] = 0;
      }
    
      for (; sy < image.h && scaley * sy < dy + 1; ++sy) {
	//      std::cout << "sy: " << sy << " from " << image.h << std::endl;
	for (int sx = 0; sx < image.w; ++sx) {
	  const int dx = std::min ((int)(scalex*sx), new_image.w-1);
	  //	std::cout << "sx: " << sx << " -> " << dx << std::endl;
	  boxes[dx] += *src; ++src;
	  ++count[dx];
	}
      }
    
      // set box
      //    std::cout << "dy: " << dy << " from " << new_image.h << std::endl;
      for (int dx = 0; dx < new_image.w; ++dx) {
	//      std::cout << "setting: dx: " << dx << ", from: " << new_image.w
	//       		<< ", count: " << count[dx] << std::endl;      
	dst.set (boxes[dx] / count[dx]);
	/* Test pattern:
	   dst.setL (dx*dy);
	   dst.set (dst); */
	++dst;
      }
    }
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
  Image image;
  image.copyTransferOwnership (new_image);
  
  new_image.New ((int)(scalex * (double) image.w),
		 (int)(scaley * (double) image.h));
  new_image.xres = (int) (scalex * image.xres);
  new_image.yres = (int) (scaley * image.yres);

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

void ddt_scale (Image& new_image, double scalex, double scaley)
{
  Image image;
  image.copyTransferOwnership (new_image);
  
  new_image.New ((int)(scalex * (double) image.w),
		 (int)(scaley * (double) image.h));
  new_image.xres = (int)(scalex * image.xres);
  new_image.yres = (int)(scaley * image.yres);
  
  // first scan the source image and build a direction map
  char dir_map [image.h][image.w];
  
  Image::iterator src_a = image.begin();
  Image::iterator src_b = src_a.at (0, 1);
  Image::iterator src_c = src_b.at (1, 0);
  Image::iterator src_d = src_c.at (1, 1);
  for (int y = 0; y < image.h-1; ++y) {
    for (int x = 0; x < image.w-1; ++x) {
      const int a = (*src_a).getL(); ++src_a;
      const int b = (*src_b).getL(); ++src_b;
      const int c = (*src_c).getL(); ++src_c;
      const int d = (*src_d).getL(); ++src_d;
      
      //std::cout << "x: " << x << ", y: " << y << std::endl;
      //std::cout << "a: " << a << ", b: " << b
      //	  << ", c: " << c << ", d: " << d << std::endl;
      
      if (abs(a-c) < abs(b-d))
	dir_map[y][x] = '\\';
      else
	dir_map[y][x] = '/';
      //std::cout << dir_map[y][x];
    }
    ++src_a; ++src_b; ++src_c; ++src_d;
    //std::cout << std::endl;
  }

  Image::iterator dst = new_image.begin();
  Image::iterator src = image.begin();
  for (int y = 0; y < new_image.h; ++y) {
    const double by = (-1.0+image.h) * y / new_image.h;
    const int sy = (int)floor(by);
    const int ydist = (int) ((by - sy) * 256);
    
    for (int x = 0; x < new_image.w; ++x) {
      const double bx = (-1.0+image.w) * x / new_image.w;
      const int sx = (int)floor(bx);
      const int xdist = (int) ((bx - sx) * 256);
      
      /*
      std::cout << "bx: " << bx << ", by: " << by << ", x: " << x << ", y: " << y
		<< ", sx: " << sx << ", sy: " << sy << std::endl;
      */
      
      Image::iterator a = src.at (sx, sy);
      Image::iterator b = src.at (sx, sy+1);
      Image::iterator c = b; ++c;
      Image::iterator d = a; ++d;
      Image::iterator v;
      
      // which triangle does the point fall into?
      if (false && dir_map[sy][sx] == '/') {
	if (xdist <= 256-ydist) // left side triangle
	  {
	    // std::cout << "/-left" << std::endl;
	    v = (
		 *a * (256-xdist) * (256-ydist) +
		 *b * (256-xdist) * ydist +
		 *d * xdist       * (256-ydist) +
		 (*b+*d) /2 * xdist * ydist
		 );
	  }
	else // right side triangle
	  {
	    //std::cout << "/-right" << std::endl;
	    v = (
		 *b * (256-xdist) * ydist +
		 *c * xdist       * ydist +
		 *d * xdist       * (256-ydist) +
		 (*b+*d) /2 * (256-xdist) * (256-ydist)
		 );
	  }
      }
      else {
	if (xdist <= ydist) // left side triangle
	  {
	    //std::cout << "\\-left" << std::endl;
	    v = (
		 *a * (256-xdist) * (256-ydist) +
		 *b * (256-xdist) * ydist +
		 *c * xdist       * ydist +
		 (*a+*c) /2 * xdist * (256-ydist)
		 );
	  }
	else // right side triangle
	  {
	    //std::cout << "\\-right" << std::endl;
	    v = (
		 *a * (256-xdist) * (256-ydist) +
		 *c * xdist       * ydist +
		 *d * xdist       * (256-ydist) +
		 (*a+*c) /2 * (256-xdist) * ydist
		 );
	  }
      }
      
      dst.set (v / (256*256) );
      ++dst;
    }
  }
}
