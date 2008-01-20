#include <iostream>
#include <algorithm>

#include "Image.hh"
#include "Codecs.hh"

#include "Colorspace.hh"

#include "crop.hh"

void crop (Image& image, unsigned int x, unsigned int y, unsigned int w, unsigned int h)
{
  // limit to valid boundaries
  x = std::min (x, (unsigned)image.w-1);
  y = std::min (y, (unsigned)image.h-1);
  
  w = std::min (w, (unsigned)image.w-x);
  h = std::min (h, (unsigned)image.h-y);

  // something to do?
  if (x == 0 && y == 0 && w == image.w && h == image.h)
    return;
  
  if (!image.isModified() && image.getCodec())
    if (image.getCodec()->crop(image, x, y, w, h))
      return;
  
  /*
    std::cerr << "after limiting: " << x << " " << y
    << " " << w << " " << h << std::endl;
  */

  // truncate the height, this is optimized for the "just height" case
  // (of e.g. fastAutoCrop)
  if (x == 0 && y == 0 && w == image.w) {
    image.setRawData (); // invalidate
    image.h = h;
    return;
  }
  
  // bit shifting is too expensive, crop at least byte-wide
  int orig_bps = image.bps;
  if (orig_bps < 8)
    colorspace_grayX_to_gray8 (image);
  
  int stride = image.Stride();
  int cut_stride = stride * w / image.w;
  
  uint8_t* dst = image.getRawData ();
  uint8_t* src = dst + stride * y + (stride * x / image.w);
  
  for (int i = 0; i < h; ++i) {
    memmove (dst, src, cut_stride);
    dst += cut_stride;
    src += stride;
  }
  
  image.setRawData (); // invalidate
  image.w = w;
  image.h = h;
 
  switch (orig_bps) {
  case 1:
    colorspace_gray8_to_gray1 (image);
    break;
  case 2:
    colorspace_gray8_to_gray2 (image);
    break;
  case 4:
    colorspace_gray8_to_gray4 (image);
    break;
  default:
    ;
  }
}

void fastAutoCrop (Image& image)
{
  if (!image.getRawData())
    return;
  
  // which value to compare against, get RGB of first pixel of the last line
  // iterator is a generic way to get RGB regardless of the bit-depth
  u_int16_t r = 0, g = 0, b = 0;
  Image::iterator it = image.begin();
  it = it.at (0, image.h - 1);
  r = 0; g = 0; b = 0;
  (*it).getRGB (&r, &g, &b);
  
  if (r != g || g != b)
    return; // not a uniform color
  
  if (r != 0 && r != 255)
    return; // not min nor max
  
  const int stride = image.Stride();
  
  // first determine the color to crop, for now we only accept full black or white
  int h = image.h-1;
  for (; h >= 0; --h) {
    // data row
    uint8_t* data = image.getRawData() + stride * h;
    
    // optimization assumption: we have an [0-8) bit-depth gray or RGB image
    // here and we just care to compare for min or max, thus we can compare
    // just the raw payload
    int x = 0;
    for (; x < stride-1; ++x)
      if (data[x] != r) {
	// std::cerr << "breaking in inner loop at: " << x << std::endl;
	break;
      }
    
    if (x != stride-1) {
      // std::cerr << "breaking in outer loop at height: " << h << " with x: " << x << " vs. " << stride << std::endl;
      break;
    }
  }
  ++h; // we are at the line that differs

  if (h == 0) // do not crop if the image is totally empty
    return;
  
  // We could just tweak the image height here, but later we might not
  // only also crop the other borders, but also benefit from lossless
  // jpeg cropping, ...  We do not explicitly check if we crop, the
  // crop function will optimize a NOP crop away for all callers.
  crop (image, 0, 0, image.w, h);
}
