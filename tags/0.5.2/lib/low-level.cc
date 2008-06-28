
#include <stdlib.h>
#include <string.h> // memcpy

#include "low-level.hh"

void deinterlace (Image& image)
{
  int i;
  const int stride = image.stride();
  const int height = image.height();
  uint8_t* deinterlaced = (uint8_t*) malloc(image.stride() * height);

  std::cerr << "deinterlace" << std::endl;
  
  for (i = 0; i < height; ++i)
    {
      const int dst_i = i / 2 + (i % 2) * (height / 2);
      std::cerr << i << " - " << dst_i << std::endl;
      uint8_t* dst = deinterlaced + stride * dst_i;
      uint8_t* src = image.getRawData() + stride * i;
      
      memcpy(dst, src, stride);
    }
  
  image.setRawData(deinterlaced);
}
