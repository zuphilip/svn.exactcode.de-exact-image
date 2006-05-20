
#include <math.h>

#include <iostream>
#include "Image.hh"

#include "Colorspace.hh"

void nearest_scale (Image& image, double scalex, double scaley)
{
  Image new_image = image;

  new_image.New ((int)(scalex * (double) image.w),
		 (int)(scaley * (double) image.h));
  new_image.xres = (int) (scalex * image.xres);
  new_image.yres = (int) (scaley * image.yres);

  Image::iterator dst = new_image.begin();
  Image::iterator src = image.begin();
  for (int y=0; y < new_image.h; ++y)
    for (int x=0; x < new_image.w; ++x) {
      int bx = (int) (((double) x) / scalex);
      int by = (int) (((double) y) / scaley);
      
      dst.set (*src.at (bx, by) );
      ++dst;
    }
  
  image = new_image;
  new_image.data = 0;
}

void bilinear_scale (Image& image, double scalex, double scaley)
{
  Image new_image = image;

  new_image.New ((int)(scalex * (double) image.w),
		 (int)(scaley * (double) image.h));
  new_image.xres = (int) (scalex * image.xres);
  new_image.yres = (int) (scaley * image.yres);

  Image::iterator dst = new_image.begin();
  Image::iterator src = image.begin();

  for (int y = 0; y < new_image.h; ++y) {
    double by = (-1.0+image.h) * y / new_image.h;
    int sy = (int)floor(by);
    int ydist = (int) ((by - sy) * 256);
    int syy = sy+1;

    for (int x = 0; x < new_image.w; ++x) {
      double bx = (-1.0+image.w) * x / new_image.w;
      int sx = (int)floor(bx);
      int xdist = (int) ((bx - sx) * 256);
      int sxx = sx+1;

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

  image = new_image;
}

void box_scale (Image& image, double scalex, double scaley)
{
  Image new_image = image;

  new_image.New ((int)(scalex * (double) image.w),
		 (int)(scaley * (double) image.h));
  
  new_image.xres = (int) (scalex * image.xres);
  new_image.yres = (int) (scaley * image.yres);
  
  /* handcrafted due popular request */
  if (new_image.bps == 8 && new_image.spp == 1)
    {
      uint8_t* src = image.data;
      uint8_t* dst = new_image.data;
      
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
      
      image = new_image;
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
  for (int sy = 0; dy < new_image.h && sy < image.h; ++dy) {
    
    // clear for accumulation
    for (int x = 0; x < new_image.w; ++x) {
      boxes[x].clear();
      count[x] = 0;
    }

    for (; sy < image.h && scaley * sy < dy + 1; ++sy) {
      //      std::cout << "sy: " << sy << " from " << image.h << std::endl;
      for (int sx = 0; sx < image.w; ++sx) {
	int dx = std::min ((int)(scalex*sx), new_image.w-1);
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
      ++dst;
    }
    
  }
  
  image = new_image;
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

void bicubic_scale (Image& image, double scalex, double scaley)
{
  Image new_image = image;

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
    int sy = std::min((int)by, image.h-1);
    int ydist = (int) ((by - sy) * 256);
    
    int sy0 = std::max(sy-1, 0);
    int sy2 = std::min(sy+1, image.h-1);
    int sy3 = std::min(sy+2, image.h-1);
    
    for (int x = 0; x < new_image.w; ++x) {
      
      double bx = .5+x / scalex;
      int sx = std::min((int)bx, image.w - 1);
      int xdist = (int) ((bx - sx) * 256);
      
      int sx0 = std::max(sx-1, 0);
      int sx2 = std::min(sx+1, image.w-1);
      int sx3 = std::min(sx+2, image.w-1);
      
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

  image = new_image;
}

void ddt_scale (Image& image, double scalex, double scaley)
{
  Image new_image = image;

  new_image.New ((int)(scalex * (double) image.w),
		 (int)(scaley * (double) image.h));
  
  new_image.xres = (int) (scalex * image.xres);
  new_image.yres = (int) (scaley * image.yres);
  
  Image::iterator dst = new_image.begin();
  Image::iterator src = image.begin();
  Image::iterator src2 = src.at(0,1);
  
  // first scan the source image and build a direction map
  char dir_map [image.h][image.w];
  for (int y = 0; y < image.h-1; ++y) {
    for (int x = 0; x < image.w-1; ++x) {
      int a = (*src).getL(); ++src;
      int b = (*src2).getL(); ++src2;
      int c = (*src2).getL(); ++src2;
      int d = (*src).getL(); ++src;
      
      std::cout << "a: " << a << ", b: " << b
		<< ", c: " << c << ", d: " << d << std::endl;
      
      if (abs(a-c) < abs(b-d))
	dir_map[y][x] = '\\';
      else
	dir_map[y][x] = '/';
      std::cout << dir_map[y][x];
    }
    std::cout << std::endl;
  }
  
  for (int y = 0; y < new_image.h; ++y) {
    double by = (-1.0+image.h) * y / new_image.h;
    int sy = (int)floor(by);
    int ydist = (int) ((by - sy) * 256);
    
    for (int x = 0; x < new_image.w; ++x) {
      double bx = (-1.0+image.w) * x / new_image.w;
      int sx = (int)floor(bx);
      int xdist = (int) ((bx - sx) * 256);

      std::cout << "bx: " << bx << ", by: " << by << ", x: " << x << ", y: " << y
		<< ", sx: " << sx << ", sy: " << sy << std::endl;

      Image::iterator a = src.at (sx, sy);
      Image::iterator d = a; ++d;
      Image::iterator b = src.at (sx, sy+1);
      Image::iterator c = b; ++c;
      Image::iterator v;
      
      // which triangle does the point fall into?
      if (dir_map[sy][sx] == '/') {
	if (xdist <= 256-ydist) // left side triangle
	  {
	    std::cout << "/-left" << std::endl;
	    v = (*a * (256-xdist) * (256-ydist) +
		 *b * (256-xdist) * ydist +
		 *d * xdist       * (256-ydist) +
		 (*b+*d) * xdist * ydist / 2);
	  }
	else // right side triangle
	  {
	    std::cout << "/-right" << std::endl;
	    v = (*b * (256-xdist) * ydist +
		 *c * xdist       * ydist +
		 *d * xdist       * (256-ydist) +
		 (*b+*d) * (256-xdist) * (256-ydist) / 2);
	  }
      }
      else {
	if (xdist <= ydist) // left side triangle
	  {
	    std::cout << "\\-left" << std::endl;
	    v = (*a * (256-xdist) * (256-ydist) +
		 *b * (256-xdist) * ydist +
		 *c * xdist       * ydist +
		 (*a+*c) * xdist * (256-ydist) /  2);
	  }
	else // right side triangle
	  {
	    std::cout << "\\-right" << std::endl;
	    v = (*a * (256-xdist) * (256-ydist) +
		 *c * xdist       * ydist +
		 *d * xdist       * (256-ydist) +
		 (*a+*c) * (256-xdist) * ydist / 2);
	  }
      }
      
      dst.set (v / (256*256) );
      ++dst;
    }
  }

  image = new_image;
}
