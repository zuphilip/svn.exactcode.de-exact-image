#include <iostream>

#include "Image.hh"

uint8_t* Image::getRawData () const {
  // TODO: ask codec about it
  return data;
}

uint8_t* Image::getRawDataEnd () const {
  // we call getRawData as it might have to query the codec to actually load it
  return getRawData() + h * Stride();
}

void Image::setRawData (uint8_t* _data) {
  // TODO: wipe the attached codec?
  if (data)
    free (data);
  data = _data;
}

void Image::setRawDataWithoutDelete (uint8_t* _data) {
  data = _data;
}

void Image::New (int _w, int _h) {
  w = _w;
  h = _h;
  data = (unsigned char*) realloc (data, Stride() * h);
}

Image::Image ()
  : data(0) {
}

Image::~Image () {
  if (data)
    free (data);
}

Image::Image (Image& other)
  : data(0)
{
  operator= (other);
}

Image& Image::operator= (Image& other)
{
  w = other.w;
  h = other.h;
  bps = other.bps;
  spp = other.spp;
  xres = other.xres;
  yres = other.yres;
  if (data)
    free (data);
  data = other.data;
  other.data = 0;
  return *this;
}

Image* Image::Clone () {
  Image* im = new Image;
  *im = *this;
  
  // currently for historic reasons the copy semantic is a bit unhandy here
  this->data = im->data;
  im->data = 0;
  im->New (im->w, im->h);
  
  // copy pixel data
  memcpy (im->data, this->data, im->Stride() * im->h);
  return im;
}
