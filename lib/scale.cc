
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
      
      dst.set (src.at (bx, by) );
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

  for (int y=0; y < new_image.h; ++y) {
    int sy = std::min((int) ((double)y / scaley), image.h - 1);
    int syy = std::min((int) (((double)y+.5) / scaley +.5), image.h - 1);
    int fy = 256 / (syy - sy);
    int fyy = 256 - fy;

    for (int x=0; x < new_image.w; ++x) {
      int sx = std::min((int) ((double)x / scalex), image.w - 1);
      int sxx = std::min((int) (((double)x+.5) / scalex +.5), image.w - 1);
      int fx = 256 / (sxx - sx);
      int fxx = 256 - fx;

      if (false && x < 8 && y < 8) {
        std::cout << "sx: " << sx << ", sy: " << sy << ", sxx: " << sxx << ", syy: " << syy << std::endl;
        std::cout << "fx: " << fx << ", fy: " << fy << ", fxx: " << fxx << ", fyy: " << fyy << std::endl;
      }

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

void box_scale (Image& image, double scalex, double scaley)
{
  Image new_image = image;

  new_image.New ((int)(scalex * (double) image.w),
		 (int)(scaley * (double) image.h));
  new_image.xres = (int) (scalex * image.xres);
  new_image.yres = (int) (scaley * image.yres);
  
  Image::iterator dst = new_image.begin();
  Image::iterator src = image.begin();
  
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
    
    for (; sy < image.h && sy * scaley < dy + 1; ++sy) {
      std::cout << "sy: " << sy << " from " << image.h << std::endl;
      for (int sx=0; sx < image.w; ++sx) {
	//	std::cout << "sx: " << sx << " -> " << (int)(sx*scalex) << std::endl;
	boxes[(int)(sx*scalex)] += src.at(sx, sy);
	++count[(int)(sx*scalex)];
      }
    }
    // set box
    std::cout << "dy: " << dy << " from " << new_image.h << std::endl;
    for (int dx = 0; dx < new_image.w; ++dx) {
      std::cout << "setting: dx: " << dx << ", count: " << count[dx] << std::endl;      
      dst.set (boxes[dx] / count[dx]);
      ++dst;
    }
    
  }
  
  image = new_image;
  new_image.data = 0;
}
