
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

bool RAWLoader::readImage (FILE* file, Image& image)
{
  if (!image.data)
    image.data = (uint8_t*) malloc (image.Stride()*image.h);
  return fread (image.data, 1, image.Stride()*image.h, file) ==
         (size_t) image.Stride()*image.h;
}

bool RAWLoader::writeImage (FILE* file, Image& image, int quality)
{
  if (!image.data)
    return false;

  return fwrite (image.data, 1, image.Stride()*image.h, file) ==
         (size_t) image.Stride()*image.h;
}

RAWLoader raw_loader;
