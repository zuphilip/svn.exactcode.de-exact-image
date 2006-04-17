
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

void linear_scale (Image& image, double scale)
{
  Image new_image = image;

  new_image.New ((int)(scale * (double) image.w),
		 (int)(scale * (double) image.h));
  new_image.xres = (int) (scale * image.xres);
  new_image.yres = (int) (scale * image.yres);

  scale = 256.0 / scale;
  
  Image::iterator dst = new_image.begin();
  Image::iterator src = image.begin();
  for (int y=0; y < new_image.h; ++y)
    for (int x=0; x < new_image.w; ++x) {
      int bx = (int) (((double) x) * scale);
      int by = (int) (((double) y) * scale);
      
      int sx = std::min(bx / 256, image.w - 1);
      int sy = std::min(by / 256, image.h - 1);
      int sxx = std::min(sx + 1, image.w - 1);
      int syy = std::min(sy + 1, image.h - 1);

      int fxx = bx % 256;
      int fyy = by % 256;
      int fx = 256 - fxx;
      int fy = 256 - fyy;
      
      dst.set ( (src.at (sx,  sy ) * fx  * fy +
		 src.at (sxx, sy ) * fxx * fy +
		 src.at (sx,  syy) * fx  * fyy +
		 src.at (sxx, syy) * fxx * fyy) / (256 * 256) );
      ++dst;
    }
  
  image = new_image;
  new_image.data = 0;
}
