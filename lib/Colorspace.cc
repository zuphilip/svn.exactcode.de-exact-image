
#if defined(__FreeBSD__) || defined(__APPLE__)
#include <machine/endian.h>
#else
#include <endian.h>
#endif

#include <iostream>

#include "Image.hh"
#include "Colorspace.hh"

void normalize (Image& image, uint8_t low, uint8_t high)
{
  int histogram[256] = { 0 };
  
  for (uint8_t* it = image.getRawData(); it < image.getRawDataEnd();)
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

  for (uint8_t* it = image.getRawData(); it < image.getRawDataEnd(); ++it)
    *it = ((int) *it * a + b) / 256;

}

void colorspace_rgb8_to_gray8 (Image& image)
{
  uint8_t* output = image.getRawData();
  for (uint8_t* it = image.getRawData(); it < image.getRawData() + image.w*image.h*image.spp;)
    {
      // R G B order and associated weighting
      int c = (int)*it++ * 28;
      c += (int)*it++ * 59;
      c += (int)*it++ * 11;
      
      *output++ = (uint8_t)(c / 100);
    }
  image.spp = 1; // converted data right now
}

void colorspace_gray8_to_gray1 (Image& image, uint8_t threshold)
{
  uint8_t *output = image.getRawData();
  uint8_t *input = image.getRawData();
  
  for (int row = 0; row < image.h; row++)
    {
      uint8_t z = 0;
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

void colorspace_gray8_to_gray4 (Image& image)
{
  uint8_t *output = image.getRawData();
  uint8_t *input = image.getRawData();
  
  for (int row = 0; row < image.h; row++)
    {
      uint8_t z = 0;
      int x = 0;
      for (; x < image.w; x++)
	{
	  z <<= 4;
	  z |= (*input++ >> 4) & 0xF;
	  
	  if (x % 2 == 1)
	    {
	      *output++ = z;
	      z = 0;
	    }
	}
      // remainder - TODO: test for correctness ...
      int remainder = 2 - x % 2;
      if (remainder != 2)
	{
	  z <<= 4*remainder;
	  *output++ = z;
	}
    }
  image.bps = 4;
}
void colorspace_gray8_to_gray2 (Image& image)
{
  uint8_t *output = image.getRawData();
  uint8_t *input = image.getRawData();
  
  for (int row = 0; row < image.h; ++row)
    {
      uint8_t z = 0;
      int x = 0;
      for (; x < image.w; x++)
	{
	  z <<= 2;
	  z |= (*input++ >> 6) & 0x3;
	  
	  if (x % 4 == 3)
	    {
	      *output++ = z;
	      z = 0;
	    }
	}
      // remainder - TODO: test for correctness ...
      int remainder = 4 - x % 4;
      if (remainder != 4)
	{
	  z <<= 2*remainder;
	  *output++ = z;
	}
    }
  image.bps = 2;
}

void colorspace_gray8_to_rgb8 (Image& image)
{
  uint8_t* data = (uint8_t*)malloc (image.w*image.h*3);
  uint8_t* output = data;
  for (uint8_t* it = image.getRawData ();
       it < image.getRawData() + image.w*image.h*image.spp; ++it)
    {
      *output++ = *it;
      *output++ = *it;
      *output++ = *it;
    }
  image.setRawData(data);
  image.spp = 3; // converted data right now
}

void colorspace_grayX_to_gray8 (Image& image)
{
  // sanity and for compiler optimization
  if (image.spp != 1)
    return;

  Image gray8_image;
  gray8_image.bps = 8;
  gray8_image.spp = 1;
  gray8_image.New (image.w, image.h);

  Image::iterator it = image.begin();
  Image::iterator gray8_it = gray8_image.begin();

  while (gray8_it != gray8_image.end()) {
    *it;
    gray8_it.setL (it.getL());
    gray8_it.set (gray8_it);
    ++it; ++gray8_it;
  }
  image = gray8_image;
}

void colorspace_grayX_to_rgb8 (Image& image)
{
  // sanity and for compiler optimization
  if (image.spp != 1)
    return;
  
  Image rgb_image;
  rgb_image.bps = 8;
  rgb_image.spp = 3;
  rgb_image.New (image.w, image.h);
  
  Image::iterator it = image.begin();
  Image::iterator rgb_it = rgb_image.begin();
  
  while (rgb_it != rgb_image.end()) {
    *it;
    rgb_it.setL (it.getL());
    rgb_it.set (rgb_it);
    ++it; ++rgb_it;
  }
  image = rgb_image;
}

void colorspace_gray1_to_gray2 (Image& image)
{
  uint8_t* data = (uint8_t*) malloc (image.h*image.w/4);
  
  uint8_t* output = data;
  uint8_t* input = image.getRawData();
  
  for (int row = 0; row < image.h; row++)
    {
      uint8_t z = 0;
      uint8_t zz = 0;
      for (int x = 0; x < image.w; x++)
	{
	  if (x % 8 == 0)
	    z = *input++;

	  zz <<= 2;
	  if (z >> 7)
	    zz |= 0x3;
	  z <<= 1;
	  
	  if (x % 4 == 3)
	    *output++ = zz;
	}
    }
  
  image.setRawData (data);
  image.bps = 2;
}

void colorspace_gray1_to_gray4 (Image& image)
{
  uint8_t* data = (uint8_t*) malloc (image.h*image.w/2);
  
  uint8_t* output = data;
  uint8_t* input = image.getRawData();
  
  for (int row = 0; row < image.h; ++row)
    {
      uint8_t z = 0;
      uint8_t zz = 0;
      for (int x = 0; x < image.w; ++x)
	{
	  if (x % 8 == 0)
	    z = *input++;
	  
	  zz <<= 4;
	  if (z >> 7)
	    zz |= 0x0F;
	  z <<= 1;
	  
	  if (x % 2 == 1)
	    *output++ = zz;
	}
    }
  
  image.setRawData (data);
  image.bps = 4;
}

void colorspace_gray1_to_gray8 (Image& image)
{
  uint8_t* data = (uint8_t*) malloc (image.h*image.w);
  
  uint8_t* output = data;
  uint8_t* input = image.getRawData();
  
  for (int row = 0; row < image.h; row++)
    {
      uint8_t z = 0;
      for (int x = 0; x < image.w; x++)
	{
	  if (x % 8 == 0)
	    z = *input++;
	  
	  *output++ = (z >> 7) ? 0xff : 0x00;
	  
	  z <<= 1;
	}
    }
  
  image.setRawData (data);
  image.bps = 8;
}

void colorspace_16_to_8 (Image& image)
{
  uint8_t* output = image.getRawData();
  for (uint8_t* it = image.getRawData();
       it < image.getRawDataEnd();)
    {
#if __BYTE_ORDER != __BIG_ENDIAN
      *output++ = it[1];
#else
      *output++ = it[0];
#endif
      it += 2;
    }
  image.bps = 8; // converted 8bit data
}

void colorspace_8_to_16 (Image& image)
{
  uint16_t* data = (uint16_t*) malloc (image.w*image.h*image.spp*2);
  uint16_t* out_it = data;
  for (uint8_t* it = image.getRawData();
       it < image.getRawDataEnd();)
    {
      *out_it++ = *it++ * 0xffff / 255;
    }
  image.setRawData ((uint8_t*)data);
  image.bps = 16; // converted 16bit data
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
	for (uint8_t* it = image.getRawData();
	     it < image.getRawDataEnd();
	     ++it)
	  *it ^= 0xff;
	return;
      }
  }
  
  // detect gray tables
  bool is_gray = false;
  if (table_entries > 1) {
    int i;
    for (i = 0; i < table_entries; ++i) {
      if (rmap[i]>>8 != gmap[i]>>8 != bmap[i]>>8 != i * 256 / (table_entries-1))
	break;
    }
    
    if (i == table_entries) {
      std::cerr << "found gray table." << std::endl;
      is_gray = true;
    }
    
    // shortpath if the data is already 8bit gray
    if (is_gray && (image.bps == 8 || image.bps == 4 || image.bps == 2) ) {
      std::cerr << "is gray table" << std::endl;
      return;
    }
  }
  
  int new_size = image.w * image.h;
  if (!is_gray) // RGB
    new_size *= 3;
  
  uint8_t* orig_data = image.getRawData();
  uint8_t* new_data = (uint8_t*) malloc (new_size);
  
  uint8_t* src = orig_data;
  uint8_t* dst = new_data;

  // TODO: allow 16bit output if the palette contains that much dynamic

  int bits_used = 0;
  int x = 0;
  while (dst < new_data + new_size)
    {
      uint8_t v = *src >> (8 - image.bps);
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
  image.setRawData (new_data);
  
  image.bps = 8;
  if (is_gray)
    image.spp = 1;
  else
    image.spp = 3;
}
