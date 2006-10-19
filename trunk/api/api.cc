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

#include "api.hh"

Image* newImage ()
{
  return new Image;
}

void deleteImage (Image* image)
{
  delete image;
}

Image* copyImage (Image* other)
{
  Image* im = new Image;
  *im = *other;
  
  // currently for historic reasons the copy semantic is a bit unhandy here
  other->data = im->data;
  im->data = 0;
  im->New (im->w, im->h);
  
  // copy pixel data
  memcpy (im->data, other->data, im->Stride() * im->h);
  
  return im;
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

char* imageColorspace (Image* image)
{
  switch (image->spp * image->bps)
    {
    case 1: return "gray1";
    case 2: return "gray2";
    case 4: return "gray4";
    case 8: return "gray8";
    case 16: return "gray16";
    case 24: return "rgb8";
    case 48: return "rgb16";
    default: return "";
    }
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

bool imageConvertColorspace (Image* image, const char* target_colorspace)
{
  std::string space = target_colorspace;
  int spp, bps;
  if (space == "bw" || space == "gray1") {
    spp = 1; bps = 1;
  } else if (space == "gray2") {
    spp = 1; bps = 2;
  } else if (space == "gray4") {
    spp = 1; bps = 4;
  } else if (space == "gray" || space == "gray8") {
    spp = 1; bps = 8;
  } else if (space == "gray16") {
    spp = 1; bps = 16;
  } else if (space == "rgb" || space == "rgb8") {
    spp = 3; bps = 8;
  } else if (space == "rgb16") {
    spp = 3; bps = 16;
  // TODO: CYMK, YVU, ...
  } else {
    std::cerr << "Requested colorspace conversion not yet implemented."
              << std::endl;
    return false;
  }

  // no image data, e.g. for loading raw images
  if (!image->data) {
    image->spp = spp;
    image->bps = bps;
    return true;
  }

  // up
  if (image->bps == 1 && bps == 2)
    colorspace_gray1_to_gray2 (*image);
  else if (image->bps == 1 && bps == 4)
    colorspace_gray1_to_gray4 (*image);
  else if (image->bps < 8 && bps >= 8)
    colorspace_grayX_to_gray8 (*image);

  // upscale to 8 bit even for sub byte gray since we have no inter sub conv., yet
  if (image->bps < 8 && image->bps > bps)
    colorspace_grayX_to_gray8 (*image);
  
  if (image->bps == 8 && image->spp == 1 && spp == 3)
    colorspace_gray8_to_rgb8 (*image);

  if (image->bps == 8 && bps == 16)
    colorspace_8_to_16 (*image);
  
  // down
  if (image->bps == 16 && bps < 16)
    colorspace_16_to_8 (*image);
 
  if (image->spp == 3 && spp == 1) 
    colorspace_rgb8_to_gray8 (*image);

  if (spp == 1 && image->bps > bps) {
    if (image->bps == 8 && bps == 1)
      colorspace_gray8_to_gray1 (*image);
    else if (image->bps == 8 && bps == 2)
      colorspace_gray8_to_gray2 (*image);
    else if (image->bps == 8 && bps == 4)
      colorspace_gray8_to_gray4 (*image);
  }

  if (image->spp != spp || image->bps != bps) {
    std::cerr << "Incomplete colorspace conversion. Requested: spp: "
              << spp << ", bps: " << bps
              << " - now at spp: " << image->spp << ", bps: " << image->bps
              << std::endl;
    return false;
  }
  return true;
}

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
		       int radius, double sd, int target_dpi)
{
  optimize2bw (*image, low, high, 0, // sloppy thr
	       radius, sd);
  
  if (target_dpi && image->xres)
    {
      double scale = (double)(target_dpi) / image->xres;
      std::cerr << "scale: " << scale << std::endl;
      if (scale < 1.0)
	box_scale (*image, scale, scale);
      else
	bilinear_scale (*image, scale, scale);
    }
  
  colorspace_gray8_to_gray1 (*image, threshold);
}

bool imageIsEmpty (Image* image, double percent, int margin)
{
  return detect_empty_page (*image, percent, margin);
}

#if WITHBARDECODE == 1

#include "bardecode.hh"

char** imageDecodeBarcodes (Image* image, const char* codes,
			    int min_length, int max_length)
{
  std::vector<std::string> ret = 
    decodeBarcodes (*image, codes, min_length, max_length);
  
  char** cret = (char**)malloc (sizeof(char*) * (ret.size()+1));
  
  int i = 0;
  for (std::vector<std::string>::iterator it = ret.begin();
       it != ret.end(); ++it)
    cret[i++] = strdup (it->c_str());
  cret[i] = 0;
  
  return (char**)cret;
}

#endif
