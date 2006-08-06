
/*
 * Copyright (C) 2006 René Rebe
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2. A copy of the GNU General
 * Public License can be found in the file LICENSE.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANT-
 * ABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details.
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ImfIO.h>

#include <ImfInputFile.h>
#include <ImfOutputFile.h>

#include <ImfRgbaFile.h>
#include <ImfArray.h>
#include <IexThrowErrnoExc.h>

#include <algorithm>

#include "openexr.hh"

#include "Colorspace.hh"

using namespace Imf;
using namespace Imath;

using std::cout;
using std::endl;

class C_IStream : public IStream
{
public:
  C_IStream (FILE *file, const char fileName[])
    : IStream (fileName), _file (file) {
  }
  
  virtual bool read (char c[], int n)
  {
    if (n != (int)fread (c, 1, n, _file))
      {
	if (ferror (_file))
	  Iex::throwErrnoExc();
	else
	  throw Iex::InputExc ("Unexpected end of file.");
      }
    return feof (_file);
  }
    
  virtual Int64 tellg ()
  {
    return ftell (_file);
  }
  
  virtual void seekg (Int64 pos)
  {
    clearerr (_file);
    fseek(_file, pos, SEEK_SET);
  }
  
  virtual void clear ()
  {
    clearerr (_file);
  }
  
private:
  FILE* _file;
};

class C_OStream : public OStream
{
public:
  C_OStream (FILE *file, const char fileName[])
    : OStream (fileName), _file (file) {
  }
  
  virtual void write (const char c[], int n)
  {
    if (n != (int)fwrite (c, 1, n, _file))
      {
	if (ferror (_file))
	  Iex::throwErrnoExc();
	else
	  throw Iex::InputExc ("Unexpected end of file.");
      }
  }
    
  virtual Int64 tellp ()
  {
    return ftell (_file);
  }
  
  virtual void seekp (Int64 pos)
  {
    clearerr (_file);
    fseek(_file, pos, SEEK_SET);
  }
  
  virtual void clear ()
  {
    clearerr (_file);
  }
  
private:
  FILE* _file;
};

bool OpenEXRLoader::readImage (FILE* file, Image& image)
{
  C_IStream istream (file, "");
  
  RgbaInputFile exrfile (istream);
  Box2i dw = exrfile.dataWindow ();
  
  image.spp = 4;
  image.bps = 16;
  
  image.New (dw.max.x - dw.min.x + 1, dw.max.y - dw.min.y + 1);
  
  Array2D<Rgba> pixels (1, image.w); // working data
  
  uint16_t* it = (uint16_t*) image.data;
  for (int y = 0; y < image.h; ++y)
    {
      exrfile.setFrameBuffer (&pixels[0][0] - y * image.w, 1, image.w);
      exrfile.readPixels (y, y);
      
      for (int x = 0; x < image.w; ++x) {
	double r = pixels[0][x].r;
	double g = pixels[0][x].g;
	double b = pixels[0][x].b;
	double a = pixels[0][x].a;
	
	r = std::min (std::max (r,0.0),1.0) * 0xFFFF;
	g = std::min (std::max (g,0.0),1.0) * 0xFFFF;
	b = std::min (std::max (b,0.0),1.0) * 0xFFFF;
	a = std::min (std::max (a,0.0),1.0) * 0xFFFF;
	
	*it++ = (int) r; *it++ = (int)g; *it++ = (int)b; *it++ = (int)a;
      }
    }
  
  return true;
}

bool OpenEXRLoader::writeImage (FILE* file, Image& image,
				int quality, const std::string& compress)
{
  RgbaChannels type;
  switch (image.spp) {
  case 1:
    type = WRITE_Y; break;
  case 2:
    type = WRITE_YA; break;
  case 3:
    type = WRITE_RGB; break;
  case 4:
    type = WRITE_RGBA; break;
    break;
  default:
    std::cerr << "Unsupported image format." << std::endl;
    return false;
  }
      
  
  Box2i displayWindow (V2i (0, 0), V2i (image.w - 1, image.h - 1));
  
  C_OStream ostream (file, "");
    
  Header header (image.w, image.h);
  RgbaOutputFile exrfile (ostream, header, type);
  
  Array2D<Rgba> pixels (1, image.w); // working data
  
  uint16_t* it = (uint16_t*) image.data;
  for (int y = 0; y < image.h; ++y)
    {
      exrfile.setFrameBuffer (&pixels[0][0] - y * image.w, 1, image.w);
      
      for (int x = 0; x < image.w; ++x) {
	pixels[0][x].r = (double)*it++ / 0xFFFF;
	pixels[0][x].g = (double)*it++ / 0xFFFF;
	pixels[0][x].b = (double)*it++ / 0xFFFF;
	pixels[0][x].a = (double)*it++ / 0xFFFF;
      }
      
      exrfile.writePixels (1);
    }
  
  return true;
}

OpenEXRLoader openexr_loader;
