
#include <string>
#include <sstream>
#include <iostream>

#include <Image.hh>
#include <Codecs.hh>

#include <api.hh>

Image* decodeImage (const char* data, int n)
{
  const std::string str (data, n); 
  std::istringstream stream (str);
  
  Image* image = new Image;
  if (ImageCodec::Read (&stream, *image))
    return image;
  
  delete image;
  return 0;
}

char* encodeImage (Image* image, const char* codec, int quality, const char* compression)
{
  /*
    std::string str ();
  std::ostringstream stream (str);
  
  ImageCodec::Write (&stream, *image, codec, ""; quality, compression);
  
  return "";
  */
}

Image* decodeImageFile (const char* filename)
{
  Image* image = new Image;
  ImageCodec::Read (filename, *image);
  
  return image;
}

bool encodeImageFile (Image* image, const char* filename)
{
}

  
