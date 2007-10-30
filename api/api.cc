#include <math.h>

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
#include <ContourMatching.hh>

#include <vectorial.hh>

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

static Image::iterator background_color;
static Image::iterator foreground_color;

bool imageConvertColorspace (Image* image, const char* target_colorspace)
{
  return colorspace_by_name (*image, target_colorspace);
}

void imageRotate (Image* image, double angle)
{
  rotate (*image, angle, background_color);
}

Image* copyImageCropRotate (Image* image, unsigned int x, unsigned int y,
			   unsigned int w, unsigned int h, double angle)
{
  return copy_crop_rotate (*image, x, y, w, h, angle, background_color);
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

void imageCrop (Image* image, unsigned int x, unsigned int y, unsigned int w, unsigned int h)
{
  crop (*image, x, y, w, h);
}

void imageFastAutoCrop (Image* image)
{
  fastAutoCrop (*image);
}

// color controlls

void setForegroundColor (double r, double g, double b)
{
  foreground_color.setRGB ((uint16_t)(r*0xff), (uint16_t)(g*0xff), (uint16_t)(b*0xff));
}

void setBackgroundColor (double r, double g, double b)
{
  background_color.setRGB ((uint16_t)(r*0xff), (uint16_t)(g*0xff), (uint16_t)(b*0xff));
}

// vector elements

static drawStyle style;

void setLineWidth (double width)
{
  style.width = width;
}

void imageDrawLine (Image* image, double x, double y, double x2, double y2)
{
  drawLine (*image, x, y, x2, y2, foreground_color, style);
}

void imageDrawRectangle (Image* image, double x, double y, double x2, double y2)
{
  drawRectangle (*image, x, y, x2, y2, foreground_color, style);
}

void imageDrawText (Image* image, double x, double y, char* text, double height)
{
  drawText (*image, x, y, text, height, foreground_color);
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


Contours* newContours(Image* image, int low, int high,
		      int threshold,
		      int radius, double standard_deviation)
{
  optimize2bw (*image, low, high, threshold, 0, radius, standard_deviation);
  if (threshold==0)
    threshold=200;
  FGMatrix m(*image, threshold);
  return new Contours(m);
}

void deleteContours(Contours* contours)
{
  delete contours;
}

LogoRepresentation* newRepresentation(Contours* logo_contours,
			    int max_feature_no,
			    int max_avg_tolerance,
			    int reduction_shift,
			    double maximum_angle,
			    double angle_step)
{
  return new LogoRepresentation(logo_contours,
				max_feature_no,
				max_avg_tolerance,
				reduction_shift,
				maximum_angle,
				angle_step);
}

void deleteRepresentation(LogoRepresentation* representation)
{
  delete representation;
}

double matchingScore(LogoRepresentation* representation, Contours* image_contours)
{
  return representation->Score(image_contours);
}

// theese are valid after call to matchingScore()
double logoAngle(LogoRepresentation* representation)
{
  return representation->rot_angle;
}

int logoTranslationX(LogoRepresentation* representation)
{
  return representation->logo_translation.first;
}

int logoTranslationY(LogoRepresentation* representation)
{
  return representation->logo_translation.second;
}

int inverseLogoTranslationX(LogoRepresentation* representation, Image* image)
{
  return representation->CalculateInverseTranslation(image->w/2, image->h/2).first;
}

int inverseLogoTranslationY(LogoRepresentation* representation, Image* image)
{
  return representation->CalculateInverseTranslation(image->w/2, image->h/2).second;
}


void drawMatchedContours(LogoRepresentation* representation, Image* image)
{
  int tx=representation->logo_translation.first;
  int ty=representation->logo_translation.second;
  double angle=M_PI * representation->rot_angle / 180.0;

  for (unsigned int i=0; i<representation->mapping.size(); i++) {
    double trash;
    Contours::Contour transformed;
    RotCenterAndReduce(*(representation->mapping[i].first), transformed, angle, 0, 0, trash, trash);
    DrawTContour(*image, transformed, tx, ty, 0,0,255);
    DrawContour(*image, *(representation->mapping[i].second), 0,255,0);
  }
}


void imageInvert (Image* image)
{
  invert (*image);
}

void imageBrightnessContrastGamma (Image* image, double brightness, double contrast, double gamma)
{
  brightness_contrast_gamma (*image, brightness, contrast, gamma);
}

void imageHueSaturationLightness (Image* image, double hue, double saturation, double lightness)
{
  hue_saturation_lightness (*image, hue, saturation, lightness);
}
