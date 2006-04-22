
#include <iostream>
#include "Image.hh"

#include "Colorspace.hh"

void nearest_scale (Image& image, double scale)
{
  Image new_image = image;

  new_image.New ((int)(scale * (double) image.w),
		 (int)(scale * (double) image.h));
  new_image.xres = (int) (scale * image.xres);
  new_image.yres = (int) (scale * image.yres);

  Image::iterator dst = new_image.begin();
  Image::iterator src = image.begin();
  for (int y=0; y < new_image.h; ++y)
    for (int x=0; x < new_image.w; ++x) {
      int bx = (int) (((double) x) / scale);
      int by = (int) (((double) y) / scale);
      
      dst.set (src.at (bx, by) );
      ++dst;
    }
  
  image = new_image;
  new_image.data = 0;
}

void bilinear_scale (Image& image, double scale)
{
  Image new_image = image;

  new_image.New ((int)(scale * (double) image.w),
		 (int)(scale * (double) image.h));
  new_image.xres = (int) (scale * image.xres);
  new_image.yres = (int) (scale * image.yres);

  Image::iterator dst = new_image.begin();
  Image::iterator src = image.begin();
  for (int y=0; y < new_image.h; ++y) {
    int sy = std::min((int) ((double)y / scale), image.h - 1);
    int syy = std::min(sy + 1, image.h - 1);
    
    for (int x=0; x < new_image.w; ++x) {
      int sx = std::min((int) ((double)x / scale), image.w - 1);
      int sxx = std::min(sx + 1, image.w - 1);
      
      dst.set ( (src.at (sx,  sy ) +
		 src.at (sxx, sy ) +
		 src.at (sx,  syy) +
		 src.at (sxx, syy) ) / 4 );
      ++dst;
    }
  }
  
  image = new_image;
  new_image.data = 0;
}

void box_scale (Image& image, double scale)
{
  Image new_image = image;

  new_image.New ((int)(scale * (double) image.w),
		 (int)(scale * (double) image.h));
  new_image.xres = (int) (scale * image.xres);
  new_image.yres = (int) (scale * image.yres);
  
  Image::iterator dst = new_image.begin();
  Image::iterator src = image.begin();
  
  Image::ivalue_t boxes  [new_image.w];
  
  for (int y=0; y < new_image.h; ++y) {
    int sy = std::min((int) ((double)y / scale), image.h - 1);
    int syy = std::min((int) (((double)y+.5) / scale +.5), image.h - 1);
    int fy = 256 / (syy - sy);
    int fyy = 256 - fy;

    for (int x=0; x < new_image.w; ++x) {
      int sx = std::min((int) ((double)x / scale), image.w - 1);
      int sxx = std::min((int) (((double)x+.5) / scale +.5), image.w - 1);
      int fx = 256 / (sxx - sx);
      int fxx = 256 - fx;

      
      dst.set ( (src.at (sx,  sy ) * fx  * fy +
		 src.at (sxx, sy ) * fxx * fy +
		 src.at (sx,  syy) * fx  * fyy +
		 src.at (sxx, syy) * fxx * fyy) / (256 * 256) );
      ++dst;
    }
  }
  
  image = new_image;
  new_image.data = 0;
}
