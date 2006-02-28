
#include <iostream>
#include "Image.hh"

void linear_scale (Image& image, double scale)
{
  Image new_image = image;
  
  new_image.New ((int)(scale * (double) image.w),
		 (int)(scale * (double) image.h));
  
  new_image.xres = (int) (scale * image.xres);
  new_image.yres = (int) (scale * image.yres);
  
  scale = 256.0 / scale;
  
  Image::iterator it = new_image.begin();
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
 
      unsigned int value
	= fx  * fy  * ( (unsigned int) image.data [sx  + image.w * sy ])
	+ fxx * fy  * ( (unsigned int) image.data [sxx + image.w * sy ])
	+ fx  * fyy * ( (unsigned int) image.data [sx  + image.w * syy])
	+ fxx * fyy * ( (unsigned int) image.data [sxx + image.w * syy]);

      value /= 256 * 256;
      value = std::min (value, (unsigned int) 255);

      *it++ = (unsigned char) value;
    }
    

  image = new_image;
  new_image.data = 0;
}
