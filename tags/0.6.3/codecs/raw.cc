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

#include <iostream>

#include "raw.hh"

bool RAWCodec::readImage (std::istream* stream, Image& image)
{
  image.resize (image.w, image.h);
  
  return (size_t) stream->readsome ((char*)image.getRawData(), image.stride()*image.h)
    == (size_t) image.stride()*image.h;
}

bool RAWCodec::writeImage (std::ostream* stream, Image& image, int quality,
			   const std::string& compress)
{
  if (!image.getRawData())
    return false;

  return stream->write ((char*)image.getRawData(), image.stride()*image.h)
    /* ==
       (size_t) image.stride()*image.h*/;
}

RAWCodec raw_loader;
