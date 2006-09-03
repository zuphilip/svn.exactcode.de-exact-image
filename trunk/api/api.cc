
#include <string>
#include <sstream>
#include <iostream>

#include <Image.hh>
#include <Codecs.hh>

#include <api.hh>

Image* newImage ()
{
  return new Image;
}

void deleteImage (Image* image)
{
  delete image;
}

bool decodeImage (Image* image, const char* data, int n)
{
  const std::string str (data, n); 
  std::istringstream stream (str);
  
  return ImageCodec::Read (&stream, *image);
}

bool decodeImageFile (Image* image, const char* filename)
{
  return ImageCodec::Read (filename, *image);
}

char* encodeImage (Image* image, const char* codec, int quality,
		   const char* compression)
{
  std::string str;
  std::ostringstream stream (str);
    
  ImageCodec::Write (&stream, *image, codec, "", yquality, compression);
  
  return 0;
}


bool encodeImageFile (Image* image, const char* filename,
		      int quality, const char* compression)
{
  return ImageCodec::Write (filename, *image, quality, compression);
}

