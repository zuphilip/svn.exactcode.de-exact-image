
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

void directional_average_scale_2x (Image& image)
{
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

  for (int y=0; y < new_image.h; ++y) {
    double by = .5+y / scaley;
    int sy = std::min((int)by, image.h - 1);
    int syy = std::min(sy+1, image.h - 1);
    int ydist = (int) ((by - sy) * 256);

    for (int x=0; x < new_image.w; ++x) {
      double bx = .5+x / scalex;
      int sx = std::min((int)bx, image.w - 1);
      int sxx = std::min(sx+1, image.w - 1);
      int xdist = (int) ((bx - sx) * 256);

      if (x < 8 && y < 8) {
        std::cout << "sx: " << sx << ", sy: " << sy << ", sxx: " << sxx << ", syy: " << syy << std::endl;
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
      u_int8_t* src = image.data;
      u_int8_t* dst = new_image.data;
      
      u_int32_t boxes[new_image.w];
      u_int32_t count[new_image.w];

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

void bicubic_scale (Image& image, double scalex, double scaley)
{
  Image new_image = image;

  new_image.New ((int)(scalex * (double) image.w),
		 (int)(scaley * (double) image.h));
  new_image.xres = (int) (scalex * image.xres);
  new_image.yres = (int) (scaley * image.yres);

  Image::iterator dst = new_image.begin();
  Image::iterator src = image.begin();

  for (int y=0; y < new_image.h; ++y) {
    double by = (double)y / scaley;
    int sy = std::min((int)by, image.h - 1);
    int syy = std::min(sy+1, image.h - 1);
    int fyy = (int) ((by - sy) * 256);
    int fy = 256 - fyy;

    for (int x=0; x < new_image.w; ++x) {
      double bx = (double)x / scalex;
      int sx = std::min((int)bx, image.w - 1);
      int sxx = std::min(sx+1, image.w - 1);
      int fxx = (int) ((bx - sx) * 256);
      int fx = 256 - fxx;

      if (false && x < 8 && y < 8) {
        std::cout << "sx: " << sx << ", sy: " << sy << ", sxx: " << sxx << ", syy: " << syy << std::endl;
        std::cout << "fx: " << fx << ", fy: " << fy << ", fxx: " << fxx << ", fyy: " << fyy << std::endl;
      }

      dst.set ( (*src.at (sx,  sy ) * fx  * fy +
                 *src.at (sxx, sy ) * fxx * fy +
                 *src.at (sx,  syy) * fx  * fyy +
                 *src.at (sxx, syy) * fxx * fyy) / (256 * 256) );
      ++dst;
    }
  }

  image = new_image;
}
