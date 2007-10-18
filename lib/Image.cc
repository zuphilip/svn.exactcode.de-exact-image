/* The Plain Old Data encapsulation of pixel, raster data.
 * Copyright (C) 2005 - 2007 René Rebe
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
 */

#include <iostream>

#define DEPRECATED
#include "Image.hh"
#include "Codecs.hh"

Image::Image ()
  : data(0), modified(false), codec(0)
{
}

Image::Image (Image& other)
  : data(0), modified(false), codec(0)
{
  operator= (other);
}

Image::~Image () {
  // release attached codec
  if (codec)
    delete (codec); codec = 0;
      
  // release POD
  if (data)
    free (data); data = 0;
}

void Image::copyMeta (const Image& other)
{
  w = other.w;
  h = other.h;
  bps = other.bps;
  spp = other.spp;
  xres = other.xres;
  yres = other.yres;
}

Image& Image::operator= (Image& other)
{
  copyMeta (other);
  
  uint8_t* d = other.getRawData();
  if (d) {
    New (w, h);
    memcpy (data, d, Stride() * h);
  }
  else {
    setRawData();
  }
  
  return *this;
}

void Image::copyTransferOwnership (Image& other)
{
  copyMeta (other);

  uint8_t* d = other.getRawData();
  other.setRawDataWithoutDelete (0);
  setRawData (d);
}

uint8_t* Image::getRawData () const {
  // ask codec about it
  if (!data && codec) {
    Image* image = const_cast<Image*>(this);
    codec->decodeNow (image);
    if (data) // if data was added
      image->modified = false;
  }
  return data;
}

uint8_t* Image::getRawDataEnd () const {
  // we call getRawData as it might have to query the codec to actually load it
  return getRawData() + h * Stride();
}

void Image::setRawData () {
  if (!modified) {
    // DEBUG
    // std::cerr << "Image modified" << std::endl;
    modified = true;
  }
}

void Image::setRawData (uint8_t* _data) {
  if (_data != data && data) {
    free (data);
    data = 0;
  }

  // reuse:
  setRawDataWithoutDelete (_data);
}

void Image::setRawDataWithoutDelete (uint8_t* _data) {
  data = _data;
  
  // reuse
  setRawData ();
}

void Image::New (int _w, int _h) {
  w = _w;
  h = _h;
  
  // reuse:
  setRawDataWithoutDelete ((uint8_t*) realloc (data, Stride() * h));
}

void Image::setDecoderID (const std::string& id) {
  decoderID = id;
}

const std::string& Image::getDecoderID () {
  return decoderID;
}

ImageCodec* Image::Image::getCodec() {
  return codec;
}

void Image::setCodec (ImageCodec* _codec) {
  codec = _codec;
  // at the moment the codec is attached the data recent
  if (codec) // not 0
    modified = false;
}

bool Image::isModified () {
  return modified;
}
