#include <iostream>
#include <algorithm>

#include "Image.hh"

#include "crop.hh"

void crop (Image& image, unsigned int x, unsigned int y, unsigned int w, unsigned int h)
{
  // something to do?
  if (x == 0 && y == 0 && w == image.w && h == image.h)
    return;
  
  // limit to valid boundaries
  x = std::min (x, (unsigned)image.w-1);
  y = std::min (x, (unsigned)image.h-1);
  
  w = std::min (x+w, (unsigned)image.w-x);
  h = std::min (y+h, (unsigned)image.h-y);
  
  // TODO: Call thru codec, e.g. optimized for JPEG
  /*
  std::cerr << "after limiting: " << x << " " << y
	    << " " << w << " " << h << std::endl;
  */
  
  // TODO: crop bottom, left and right
  
  if (x != 0 || y != 0 || w != image.w) {
    std::cerr << "crop: offset crop not yet implemented!" << std::endl;
    return;
  }
  
  // invalidate data
  image.setRawData ();
  image.h = h;
}
