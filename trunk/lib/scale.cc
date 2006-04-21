
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

  Image::iterator dst = new_image.begin();
  Image::iterator src = image.begin();
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

//      std::cout << "sx: " << sx << ", sy: " << sy << ", sxx: " << sxx << ", syy: " << syy << std::endl;
//      std::cout << "fx: " << fx << ", fy: " << fy << ", fxx: " << fxx << ", fyy: " << fyy << std::endl;
      
      dst.set ( (src.at (sx,  sy ) * fx  * fy +
		 src.at (sxx, sy ) * fxx * fy +
		 src.at (sx,  syy) * fx  * fyy +
		 src.at (sxx, syy) * fxx * fyy) / (256 * 256) );
//      std::cout << "x: " << x << ", y: " << y << ": " << (int) dst.ptr->gray << std::endl;
      ++dst;
    }
  }
  
  image = new_image;
  new_image.data = 0;
}
