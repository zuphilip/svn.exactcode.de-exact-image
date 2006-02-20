
#include <iostream>

#include "Image.hh"
#include "Colorspace.hh"

void normalize (Image& image, unsigned char low, unsigned char high)
{
  int histogram[256] = { 0 };
  for (unsigned char* it = image.data; it < image.data + image.w*image.h;)
    histogram[*it++]++;

  int lowest = 255, highest = 0;
  for (int i = 0; i <= 255; i++)
    {
      // std::cout << i << ": "<< histogram[i] << std::endl;
      if (histogram[i] > 2) // magic denoise constant
	{
	  if (i < lowest)
	    lowest = i;
	  if (i > highest)
	    highest = i;
	}
    }
  std::cerr << "lowest: " << lowest << ", highest: " << highest << std::endl;
  
  if (low)
    lowest = low;
  if (high)
    highest = high;
    
  // TODO use options
  signed int a = (255 * 256) / (highest - lowest);
  signed int b = -a * lowest;

  std::cerr << "a: " << (float) a / 256
	    << " b: " << (float) b / 256 << std::endl;

  for (unsigned char* it = image.data; it < image.data + image.w*image.h; ++it)
    *it = ((int) *it * a + b) / 256;

}

void colorspace_rgb_to_gray (Image& image)
{
  unsigned char* output = image.data;
  for (unsigned char* it = image.data; it < image.data + image.w*image.h*image.spp;)
    {
      // R G B order and associated weighting
      int c = (int)*it++ * 28;
      c += (int)*it++ * 59;
      c += (int)*it++ * 11;
      
      *output++ = (unsigned char)(c / 100);
    }
  image.spp = 1; // converted data right now
}

void colorspace_gray_to_bilevel (Image& image, unsigned char threshold)
{
  unsigned char *output = image.data;
  unsigned char *input = image.data;
  
  for (int row = 0; row < image.h; row++)
    {
      unsigned char z = 0;
      int x = 0;
      for (; x < image.w; x++)
	{
	  z <<= 1;
	  if (*input++ > threshold)
	    z |= 0x01;
	  
	  if (x % 8 == 7)
	    {
	      *output++ = z;
	      z = 0;
	    }
	}
      // remainder - TODO: test for correctness ...
      int remainder = 8 - x % 8;
      if (remainder != 8)
	{
	  z <<= remainder;
	  *output++ = z;
	}
    }
  image.bps = 1;
}
