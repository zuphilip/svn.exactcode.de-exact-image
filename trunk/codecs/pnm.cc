/*
 * C++ PNM library.
 * Copyright (c) 2006 Rene Rebe <rene@exactcode.de>
 *
 * I former times we used to use the netbpm library here, but as
 * it can not handle in memory or otherwise transferred files, allowing
 * access via the C FILE* exclusively (???!!!) we had to write our
 * own parser here ...
 *
 */

#include <iostream>
#include <string>
#include <sstream>

#ifdef __FreeBSD__
#include <machine/endian.h>
#else
#include <endian.h>
#endif

#include <byteswap.h>

#include "pnm.hh"

int getNextHeaderNumber (std::istream* stream)
{
  std::string s;
  
  int c = stream->peek ();
  switch (c) {
  case '\n':
  case '\r':
    stream->get (); // consume silently
    // comment line?
    if (stream->peek () == '#') {
      std::string str;
      std::getline (*stream, str); // consume comment line
    }
  }
  
  int i;
  *stream >> i;
  return i;
}

bool PNMCodec::readImage (std::istream* stream, Image& image)
{
  // check signature
  if (stream->peek () != 'P')
    return false;
  
  image.bps = 0;
  
  stream->get(); // consume P
  char mode = stream->peek();
  switch (mode) {
  case '1':
  case '4':
    image.bps = 1;
  case '2':
  case '5':
    image.spp = 1;
    break;
  case '3':
  case '6':
    image.spp = 3;
    break;
  default:
    stream->unget(); // P
    return false;
  }
  stream->get(); // consume format number
  
  image.w = getNextHeaderNumber (stream);
  image.h = getNextHeaderNumber (stream);
  
  int maxval = 1;
  if (image.bps != 1) {
    maxval = getNextHeaderNumber (stream);
  }
  
  image.bps = 1;
  while ( (1 << image.bps) < maxval)
    ++image.bps;
  
  // not stored in the format :-(
  image.xres = image.yres = 0;
  
  // allocate data, if necessary
  image.New (image.w, image.h);
  
  // consume the left over spaces and newline 'till the data begins
  {
    std::string str;
    std::getline (*stream, str);
  }
  
  if (mode <= '3') // ascii / plain text
    {
      Image::iterator it = image.begin ();
      for (int y = 0; y < image.h; ++y)
	{
	  for (int x = 0; x < image.w; ++x)
	    {
	      if (image.spp == 1) {
		int i;
		*stream >> i;
		
		i = i * (255 / maxval);
		it.setL (i);
	      }
	      else {
		uint16_t r, g, b;
		*stream >> r >> g >> b;
		
		it.setRGB (r, g, b);
	      }
	      
	      it.set (it);
	      ++it;
	    }
	}
    }
  else // binary data
    {
      const int stride = image.Stride ();
      const int bps = image.bps;
      
      for (int y = 0; y < image.h; ++y)
	{
	  uint8_t* dest = image.getRawData() + y * stride;
	  
	  stream->read ((char*)dest, stride);

	  // is it publically defined somewhere???
	  if (bps == 1) {
	    uint8_t* xor_ptr = dest;
	    for (int x = 0; x < image.w; x += 8)
	      *xor_ptr++ ^= 0xff;
	  }
	  
#if __BYTE_ORDER != __BIG_ENDIAN
	  if (bps == 16) {
	    uint16_t* swap_ptr = (uint16_t*)dest;
	    for (int x = 0; x < stride/2; ++x)
	      *swap_ptr++ = bswap_16 (*swap_ptr);
	  }
#endif
	}
    }
  
  return true;
}

bool PNMCodec::writeImage (std::ostream* stream, Image& image, int quality,
			   const std::string& compress)
{
  // ok writing should be easy ,-) just dump the header
  // and the data thereafter ,-)
  
  int format = 0;
  
  if (image.spp == 1 && image.bps == 1)
    format = 1;
  else if (image.spp == 1)
    format = 2;
  else if (image.spp == 3)
    format = 3;
  else {
    std::cerr << "Not (yet?) supported PBM format." << std::endl;
    return false;
  }
  
  std::string c (compress);
  std::transform (c.begin(), c.end(), c.begin(), tolower);
  if (c == "plain")
    c = "ascii";

  
  if (c != "ascii")
    format += 3;
  
  *stream << "P" << format << std::endl;
  *stream << "# written by ExactImage - report bugs to: <rene@exactcode.de>" << std::endl;

  *stream << image.w << " " << image.h << std::endl;
  
  // maxval
  int maxval = (1 << image.bps) - 1;
  
  if (image.bps > 1)
    *stream << maxval << std::endl;
  
  Image::iterator it = image.begin ();
  if (c == "ascii")
    {
      for (int y = 0; y < image.h; ++y)
	{
	  for (int x = 0; x < image.w; ++x)
	    {
	      *it;
	      
	      if (x != 0)
		*stream << " ";
	      
	      if (image.spp == 1) {
		*stream << it.getL() / (255 / maxval);
	      }
	      else {
		uint16_t r, g, b;
		it.getRGB (&r, &g, &b);
		*stream << (int)r << " " << (int)g << " " << (int)b;
	      }
	      ++it;
	    }
	  *stream << std::endl;
	}
    }
  else
    {
      const int stride = image.Stride ();
      const int bps = image.bps;
      
      uint8_t* ptr = (uint8_t*) malloc (stride);
      
      for (int y = 0; y < image.h; ++y)
	{
	  memcpy (ptr, image.getRawData() + y * stride, stride);
	  
	  // is this publically defined somewhere???
	  if (bps == 1) {
	    uint8_t* xor_ptr = ptr;
	    for (int x = 0; x < image.w; x += 8)
	      *xor_ptr++ ^= 0xff;
	  }
	  
#if __BYTE_ORDER != __BIG_ENDIAN
	  if (bps == 16) {
	    uint16_t* swap_ptr = (uint16_t*)ptr;
	    for (int x = 0; x < stride/2; ++x)
	      *swap_ptr++ = bswap_16 (*swap_ptr);
	  }
#endif
	  stream->write ((char*)ptr, stride);
	}
      free (ptr);
    }
  
  stream->flush ();
  
  return true;
}

PNMCodec pnm_loader;
