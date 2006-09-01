
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
  
  image.h = getNextHeaderNumber (stream);
  image.w = getNextHeaderNumber (stream);
  
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
  
  Image::iterator it = image.begin ();
  if (mode <= '3') // ascii / plain text
    {
    }
  else // binary data
    {
      char payload [3*2]; // max space we read for a pixel 3 pixel at 16 bit == 2 bytes
      int size = image.spp * (image.bps > 8 ? 2 : 1);
      
      for (int y = 0; y < image.h; ++y)
	{
	  for (int x = 0; x < image.w; ++x)
	    {
	      stream->read (payload, size);
	      
	      if (image.spp == 1) {
		int i = payload [0] * (255 / maxval);
		
		it.setL (i);
	      }
	      else {
		uint16_t r, g, b;
		
		if (size == 3) {
		  r = payload [0];
		  g = payload [1];
		  b = payload [2];
		}
		else {
		  r = (payload [0] << 8) + payload [1];
		  g = (payload [2] << 8) + payload [3];
		  b = (payload [4] << 8) + payload [5];
		}
		it.setRGB (r, g, b);
	      }
	      
	      it.set (it);
	      ++it;
	    }
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
      char payload [3*2]; // max space we consume 3 pixel at 16 bit == 2 bytes
      int size = image.spp * (image.bps > 8 ? 2 : 1);
      
      for (int y = 0; y < image.h; ++y)
	{
	  for (int x = 0; x < image.w; ++x)
	    {
	      *it;
	      
	      if (image.spp == 1) {
		int i = it.getL() / (255 / maxval);
		
		if (size == 1)
		  payload [0] = i;
		else {
		  payload [0] = i >> 8;
		  payload [1] = i & 0xff;
		}
	      }
	      else {
		uint16_t r, g, b;
		it.getRGB (&r, &g, &b);
		
		if (size == 3) {
		  payload [0] = r;
		  payload [1] = g;
		  payload [2] = b;
		}
		else {
		  payload [0] = r >> 8;
		  payload [1] = r & 0xFF;
		  payload [2] = g >> 8;
		  payload [3] = g & 0xFF;
		  payload [4] = b >> 8;
		  payload [5] = b & 0xFF;
		}
	      }
	      ++it;
	      stream->write (payload, size);
	    }
	}
    }
  
  stream->flush ();
  
  return true;
}

PNMCodec pnm_loader;
