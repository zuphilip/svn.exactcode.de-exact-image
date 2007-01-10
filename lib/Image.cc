#include <iostream>

#include "Image.hh"
#include "Codecs.hh"

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
    std::cerr << "Image modified" << std::endl;
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
