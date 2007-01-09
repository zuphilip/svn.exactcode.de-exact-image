#include <iostream>

#include "Image.hh"

Image::Image ()
  : data(0), modified(false) {
}

Image::Image (Image& other)
  : data(0), modified(false)
{
  operator= (other);
}

Image::~Image () {
  if (data)
    free (data);
}

Image& Image::operator= (Image& other)
{
  // TODO: rethink
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
  // TODO: rethink
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

uint8_t* Image::getRawData () const {
  // TODO: ask codec about it
  return data;
}

uint8_t* Image::getRawDataEnd () const {
  // we call getRawData as it might have to query the codec to actually load it
  return getRawData() + h * Stride();
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
  if (!modified) {
    std::cerr << "Image modified" << std::endl;
    modified = true;
  }
  
  data = _data;
}

void Image::New (int _w, int _h) {
  w = _w;
  h = _h;
  
  // reuse:
  setRawData ((uint8_t*) realloc (data, Stride() * h));
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
