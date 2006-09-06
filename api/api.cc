
#include <string>
#include <vector>
#include <sstream>
#include <iostream>

#include <Image.hh>
#include <Codecs.hh>

#include <rotate.hh>
#include <scale.hh>

#include <api.hh>

Image* newImage ()
{
  return new Image;
}

void deleteImage (Image* image)
{
  delete image;
}

bool decodeImage (Image* image, char* data, int n)
{
  const std::string str (data, n); 
  std::istringstream stream (str);
  
  return ImageCodec::Read (&stream, *image);
}

bool decodeImageFile (Image* image, const char* filename)
{
  return ImageCodec::Read (filename, *image);
}

void encodeImage (char **s, int *slen,
		  Image* image, const char* codec, int quality,
		  const char* compression)
{
  std::ostringstream stream (""); // empty string to start with
  
  ImageCodec::Write (&stream, *image, codec, "", quality, compression);
  stream.flush();
  
  char* payload = (char*) malloc (stream.str().size());
  
  memcpy (payload, stream.str().c_str(), stream.str().size());
  
  *s = payload;
  *slen = stream.str().size();
}


bool encodeImageFile (Image* image, const char* filename,
		      int quality, const char* compression)
{
  return ImageCodec::Write (filename, *image, quality, compression);
}


// image properties
int imageChannels (Image* image)
{
  return image->spp;
}

int imageChannelDepth (Image* image)
{
  return image->bps;
}

int imageWidth (Image* image)
{
  return image->w;
}

int imageHeight (Image* image)
{
  return image->h;
}

int imageXres (Image* image)
{
  return image->yres;
}

int imageYres (Image* image)
{
  return image->yres;
}

void imageSetXres (Image* image, int xres)
{
  image->xres = xres;
}

void imageSetYres (Image* image, int yres)
{
  image->yres = yres;
}

// image manipulation

Image::iterator background;

void imageRotate (Image* image, double angle)
{
  rotate (*image, angle, background);
}

void imageScale (Image* image, double factor)
{
  if (factor > 1.0)
    bilinear_scale (*image, factor, factor);
  else
    box_scale (*image, factor, factor);
}

void imageBoxScale (Image* image, double factor)
{
  box_scale (*image, factor, factor);
}

void imageOptimize2BW (Image* image)
{
  
}

bool imageIsEmpty (Image* image)
{
}

#ifdef WITHBARDECODE

#include "bardecode.hh"

void imageDecodeBarcodes (Image* image)
{
  decodeBarcodes (*image);
}

#endif
