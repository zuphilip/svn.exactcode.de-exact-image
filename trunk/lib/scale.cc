
#include <iostream>
#include "Image.hh"

void linear_scale (Image& image, double scale)
{
  if (image.spp != 1 || image.bps != 8)
    {
      std::cerr << "scaling with this bit-depth and colorspace not yet implemented"
		<< std::endl;
      return;
    }
  
  int wn = (int) (scale * (double) image.w);
  int hn = (int) (scale * (double) image.h);
  
  image.xres = (int) (scale * image.xres);
  image.yres = (int) (scale * image.yres);
  
  scale = 256.0 / scale;
  
  unsigned char* ndata = (unsigned char*) malloc (wn * hn);
  
  unsigned int offset = 0;
  for (int y=0; y < hn; y++)
    for (int x=0; x < wn; x++) {
      
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

      ndata[offset++] = (unsigned char) value;
    }
    
  free (image.data);
  image.data = ndata;
  image.w = wn;
  image.h = hn;
}
