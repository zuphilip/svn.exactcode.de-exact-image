
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

void colorspace_gray_to_rgb (Image& image)
{
  unsigned char* data = (unsigned char*)malloc (image.w*image.h*3);
  unsigned char* output = data;
  for (unsigned char* it = image.data;
       it < image.data + image.w*image.h*image.spp; ++it)
    {
      *output++ = *it;
      *output++ = *it;
      *output++ = *it;
    }
  free (image.data);
  image.data = data;
  image.spp = 3; // converted data right now
}

void colorspace_bilevel_to_gray (Image& image)
{
  unsigned char* data = (unsigned char*) malloc (image.h*image.w);
  
  unsigned char* output = data;
  unsigned char* input = image.data;
  
  for (int row = 0; row < image.h; row++)
    {
      unsigned char z;
      for (int x = 0; x < image.w; x++)
	{
	  if (x % 8 == 0)
	    z = *input++;
	  
	  *output++ = (z >> 7) ? 0xff : 0x00;
	  
	  z <<= 1;
	  
	}
    }
  
  free (image.data);
  image.data = data;
  
  image.bps = 8;
}

void colorspace_16_to_8 (Image& image)
{
  unsigned char* output = image.data;
  for (unsigned char* it = image.data;
       it < image.data + image.w*image.h*image.spp*2;)
    {
      *output++ = it[1];
      it += 2;
    }
  image.bps = 8; // converted 8bit data
}

void colorspace_de_palette (Image& image, int table_entries,
			    uint16_t* rmap, uint16_t* gmap, uint16_t* bmap)
{
  // detect 1bps b/w tables
  if (image.bps == 1) {
    if (rmap[0] == 0 &&
	gmap[0] == 0 &&
	bmap[0] == 0 &&
	rmap[1] >= 0xff00 &&
	gmap[1] >= 0xff00 &&
	bmap[1] >= 0xff00)
      {
	std::cerr << "correct b/w table." << std::endl;
	return;
      }
    if (rmap[1] == 0 &&
	gmap[1] == 0 &&
	bmap[1] == 0 &&
	rmap[0] >= 0xff00 &&
	gmap[0] >= 0xff00 &&
	bmap[0] >= 0xff00)
      {
	std::cerr << "inverted b/w table." << std::endl;
	for (unsigned char* it = image.data;
	     it < image.data + image.Stride()*image.h;
	     ++it)
	  *it ^= 0xff;
	return;
      }
  }
  
  // detect gray tables
  bool is_gray = false;
  {
    int i;
    for (i = 0; i < table_entries; ++i) {
      if (rmap [i] != gmap[i] ||
	  rmap [i] != bmap[i])
	break;
    }
    
    if (i == table_entries) {
      std::cerr << "found gray table." << std::endl;
      is_gray = true;
    }
    
    // shortpath if the data is already 8bit gray
    if (is_gray && image.bps == 8 && table_entries == 256) {
      std::cerr << "is gray table and already 8bit data" << std::endl;
      return;
    }
  }
  
  int new_size = image.w * image.h;
  if (!is_gray) // RGB
    new_size *= 3;
  
  unsigned char* orig_data = image.data;
  image.data = (unsigned char*) malloc (new_size);
  
  unsigned char* src = orig_data;
  unsigned char* dst = image.data;

  // TODO: allow 16bit output if the palete contains that much dynamic

  int bits_used = 0;
  int x = 0;
  while (dst < image.data + new_size)
    {
      unsigned char v = *src >> (8 - image.bps);
      if (is_gray) {
	*dst++ = rmap[v] >> 8;
      } else {
	*dst++ = rmap[v] >> 8;
	*dst++ = gmap[v] >> 8;
	*dst++ = bmap[v] >> 8;
      }
      
      bits_used += image.bps;
      ++x;

      if (bits_used == 8 || x == image.w) {
	++src;
	bits_used = 0;
	if (x == image.w)
	  x = 0;
      }
      else {
	*src <<= image.bps;
      }
    }
  free (orig_data);
  
  image.bps = 8;
  if (is_gray)
    image.spp = 1;
  else
    image.spp = 3;
}
