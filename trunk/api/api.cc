
#include <string>
#include <vector>
#include <sstream>
#include <iostream>

#include <Image.hh>
#include <Codecs.hh>

#include <rotate.hh>
#include <scale.hh>

#include <Colorspace.hh>

#include <optimize2bw.hh>
#include <empty-page.hh>

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
  bilinear_scale (*image, factor, factor);
}

void imageBoxScale (Image* image, double factor)
{
  box_scale (*image, factor, factor);
}

void imageOptimize2BW (Image* image, int low, int high,
		       int threshold,
		       int radius, double sd)
{
  optimize2bw (*image, low, high, 0, // sloppy thr
	       radius, sd);
  // optimize does not do this itself anymore
  colorspace_gray8_to_gray1 (*image, threshold);
}

bool imageIsEmpty (Image* image, double percent, int margin)
{
  return detect_empty_page (*image, percent, margin);
}

#ifdef WITHBARDECODE

#include "bardecode.hh"

char** imageDecodeBarcodes (Image* image, const char* codes,
			    int min_length, int max_length)
{
  std::vector<std::string> ret = 
    decodeBarcodes (*image, codes, min_length, max_length);
  
  char** cret = (char**)malloc (sizeof(char*) * (ret.size()+1));
  
  int i = 0;
  for (std::vector<std::string>::iterator it = ret.begin();
       it != ret.end(); ++it, ++i)
    {
      cret[i] = (char*)malloc (it->length()+1);
      strcpy (cret[i], it->c_str());
    }
  cret[i] = 0;
  
  return (char**)cret;
}

#endif
