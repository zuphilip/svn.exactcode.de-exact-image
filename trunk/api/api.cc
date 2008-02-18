/*
 * The ExactImage stable external API for use with SWIG.
 * Copyright (C) 2006 - 2008 René Rebe, ExactCODE GmbH
 * Copyright (C) 2006 René Rebe, Archivista
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2. A copy of the GNU General
 * Public License can be found in the file LICENSE.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANT-
 * ABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details.
 * 
 */

#include <math.h>

#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <sstream>

#include <Image.hh>
#include <Codecs.hh>

#include <rotate.hh>
#include <scale.hh>
#include <crop.hh>

#include <Colorspace.hh>

#include <optimize2bw.hh>
#include <empty-page.hh>
#include <ContourMatching.hh>

#include "Tokenizer.hh"	// barcode decoding
#include "Scanner.hh"

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

const std::string encodeImage (Image* image, const char* codec, int quality,
                         const char* compression)
{
  std::ostringstream stream (""); // empty string to start with

  ImageCodec::Write (&stream, *image, codec, "", quality, compression);
  stream.flush();

  return stream.str();
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

const char* imageColorspace (Image* image)
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

class drawStyle
{
public:
  drawStyle ()
    : width (1) {
  }
  
  double width;
  std::vector <double> dash;
} style;

void setLineWidth (double width)
{
  style.width = width;
}

void imageDrawLine (Image* image, double x, double y, double x2, double y2)
{
  Path path;
  path.moveTo (x, y);
  path.addLineTo (x2, y2);

  path.setLineWidth (style.width);
  path.setLineDash (0, style.dash);
  double r, g, b;
  foreground_color.getRGB (r, g, b);
  path.setFillColor (r, g, b);
  
  path.draw (*image);
}

void imageDrawRectangle (Image* image, double x, double y, double x2, double y2)
{
  Path path;
  path.addRect (x, y, x2, y2);
  path.setLineWidth (style.width);
  path.setLineDash (0, style.dash);
  path.setLineJoin (agg::miter_join);
  
  double r, g, b;
  background_color.getRGB (r, g, b);
  path.setFillColor (r, g, b);
  
  path.draw (*image);
}

void imageDrawText (Image* image, double x, double y, char* text, double height)
{
  Path path;
  double r, g, b;
  foreground_color.getRGB (r, g, b);
  path.setFillColor (r, g, b);
  
  path.drawText (*image, x, y, text, height);
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

// barcode recognition

#if WITHBARDECODE == 1

#include "bardecode.hh"

char** imageDecodeBarcodesExt (Image* image, const char* c,
			    int min_length, int max_length, int multiple)
{
  std::string codes = c;
  std::transform (codes.begin(), codes.end(), codes.begin(), tolower);

  std::vector<std::string> ret = 
    decodeBarcodes (*image, codes, min_length, max_length, multiple);
  
  char** cret = (char**)malloc (sizeof(char*) * (ret.size()+1));
  
  int i = 0;
  for (std::vector<std::string>::iterator it = ret.begin();
       it != ret.end(); ++it)
    cret[i++] = strdup (it->c_str());
  cret[i] = 0;
  
  return (char**)cret;
}

#endif

using namespace BarDecode;

namespace {

    struct comp {
        bool operator() (const scanner_result_t& a, const scanner_result_t& b) const
        {
            if (a.type < b.type) return true;
            else if (a.type > b.type) return false;
            else return (a.code < b.code);
        }
    };

    std::string filter_non_printable(const std::string& s)
    {
        std::string result;
        for (size_t i = 0; i < s.size(); ++i) {
            if ( std::isprint(s[i]) ) result.push_back(s[i]);
        }
        return result;
    }

}

char** imageDecodeBarcodes (Image* image, const char* codestr,
			    int min_length, int max_length, int multiple)
{
  codes_t codes;
  // parse the code list
  std::string c (codestr);
  std::transform (c.begin(), c.end(), c.begin(), tolower);
  std::string::size_type it = 0;
  std::string::size_type it2;
  do
    {
      it2 = c.find ('|', it);
      std::string code;
      if (it2 !=std::string::npos) {
	code = c.substr (it, it2-it);
	it = it2 + 1;
      }
      else
	code = c.substr (it);
      
      if (!code.empty())
	{
	  if (code == "code39")
	    codes |= code39;
	  else if (code == "code128")
	    codes |= code128 | gs1_128;
	  else if (code == "code25")
	    codes |= code25i;
	  else if (code == "ean13")
	    codes |= ean13;
	  else if (code == "ean8")
	    codes |= ean8;
	  else if (code == "upca")
	    codes |= upca;
	  else if (code == "upce")
	    codes |= upce;
          else if (code == "any") {
	    codes |= ean|code128|gs1_128|code39|code25i;
	  }
	  else
	    std::cerr << "Unrecognized barcode type: " << code << std::endl;
	}
    }
  while (it2 != std::string::npos);

  const int threshold = 150;
  const directions_t directions = (directions_t) 15; /* all */
  const int concurrent_lines = 4;
  const int line_skip = 8;

  std::map<scanner_result_t,int,comp> retcodes;
  if ( directions&(left_right|right_left) ) {
    BarDecode::BarcodeIterator<> it(image, threshold, codes, directions, concurrent_lines, line_skip);
    while (! it.end() ) {
      ++retcodes[*it];
      ++it;
      }
  }

  if ( directions&(top_down|down_top) ) {
       directions_t dir = (directions_t) ((directions&(top_down|down_top))>>1);
       BarDecode::BarcodeIterator<true> it(image, threshold, codes, dir, concurrent_lines, line_skip);
       while (! it.end() ) {
         ++retcodes[*it];
         ++it;
       }
  }
  
	std::vector<std::string> ret;
  for (std::map<scanner_result_t,int>::const_iterator it = retcodes.begin();
	   it != retcodes.end();
	   ++it) {
    if (it->first.type&(ean|code128|gs1_128) || it->second > 1)
	  {
		  std::stringstream s; s << it->first.type;
      ret.push_back (filter_non_printable(it->first.code));
			ret.push_back (s.str());
	  }
  }

  char** cret = (char**)malloc (sizeof(char*) * (ret.size()+1));
  
  int i = 0;
  for (std::vector<std::string>::iterator it = ret.begin();
       it != ret.end(); ++it)
    cret[i++] = strdup (it->c_str());
  cret[i] = 0;
  
  return (char**)cret;
}
