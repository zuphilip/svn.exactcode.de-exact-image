#include <string>
#include <vector>
#include <sstream>
#include <iostream>

#include <Image.hh>
#include <Codecs.hh>

#include <rotate.hh>
#include <scale.hh>
#include <crop.hh>

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
  Image* image = new Image;
  *image = *other;
  return image;
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
  return colorspace_by_name (*image, target_colorspace);
}

void imageRotate (Image* image, double angle)
{
  rotate (*image, angle, background);
}

void imageFlipX (Image* image)
{
  flipX (*image);
}

void imageFlipY (Image* image)
{
  flipY (*image);
}

void imageScale (Image* image, double factor)
{
  scale (*image, factor, factor);
}

void imageBoxScale (Image* image, double factor)
{
  box_scale (*image, factor, factor);
}

void imageNearestScale (Image* image, double factor)
{
  nearest_scale (*image, factor, factor);
}

void imageBilinearScale (Image* image, double factor)
{
  bilinear_scale (*image, factor, factor);
}

void imageFastAutoCrop (Image* image)
{
  // check for valid image
  if (!image)
    return;
  
  if (!image->getRawData())
    return;
  
  // which value to compare against, get RGB of first pixel of the last line
  // iterator is a generic way to get RGB regardless of the bit-depth
  u_int16_t r = 0, g = 0, b = 0;
  Image::iterator it = image->begin();
  it = it.at (0, image->h - 1);
  r = 0; g = 0; b = 0;
  (*it).getRGB (&r, &g, &b);
  
  if (r != g || g != b)
    return; // not a uniform color
  
  if (r != 0 && r != 255)
    return; // not min nor max
  
  const int stride = image->Stride();
  
  // first determine the color to crop, for now we only accept full black or white
  int h = image->h-1;
  for (; h >= 0; --h) {
    // data row
    uint8_t* data = image->getRawData() + stride * h;
    
    // optimization assumption: we have an [0-8) bit-depth gray or RGB image
    // here and we just care to compare for min or max, thus we can compare
    // just the raw payload
    int x = 0;
    for (; x < stride-1; ++x)
      if (data[x] != r) {
	// std::cerr << "breaking in inner loop at: " << x << std::endl;
	break;
      }
    
    if (x != stride-1) {
      // std::cerr << "breaking in outer loop at height: " << h << " with x: " << x << " vs. " << stride << std::endl;
      break;
    }
  }
  ++h; // we are at the line that differs

  if (h == 0) // do not crop if the image is totally empty
    return;
  
  // We could just tweak the image height here, but later we might not
  // only also crop the other borders, but also benefit from lossless
  // jpeg cropping, ...  We do not explicitly check if we crop, the
  // crop function will optimize a NOP crop away for all callers.
  crop (*image, 0, 0, image->w, h);
}

void imageOptimize2BW (Image* image, int low, int high,
		       int threshold,
		       int radius, double sd, int target_dpi)
{
  optimize2bw (*image, low, high, threshold, 0 /* sloppy thr */,
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
 
  /* This does not look very dynamic, but it is - the real work is
     done inside the optimize2bw library - this just yields the final
     bi-level data */

  if (!threshold)
    threshold = 200;
  
  colorspace_gray8_to_gray1 (*image, threshold);
}

bool imageIsEmpty (Image* image, double percent, int margin)
{
  return detect_empty_page (*image, percent, margin);
}

#if WITHBARDECODE == 1

#include "bardecode.hh"

char** imageDecodeBarcodes (Image* image, const char* c,
			    int min_length = 0, int max_length = 0)
{
  std::string codes = c;
  std::transform (codes.begin(), codes.end(), codes.begin(), tolower);

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
