/*
 * The ExactImage library's convert compatible command line frontend.
 * Copyright (C) 2005 - 2008 René Rebe, ExactCODE GmbH
 * Copyright (C) 2005, 2008 Archivista GmbH
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

#include <iostream>
#include <iomanip>

#include <algorithm>

#include <list>

#include "config.h"

#include "ArgumentList.hh"

#include "Image.hh"
#include "Codecs.hh"

#include "Colorspace.hh"

#include "low-level.hh"

#include "scale.hh"
#include "crop.hh"
#include "rotate.hh"
#include "deskew.hh"

#include "Matrix.hh"

#include "riemersma.h"
#include "floyd-steinberg.h"

#include "vectorial.hh"
#include "GaussianBlur.hh"

/* Let's reuse some parts of the official, stable API to avoid
 * duplicating code.
 *
 * This also includes the foreground/background color and vector
 * drawing style.
 */

#include "api/api.cc"

#include "C.h"

#include <functional>

using namespace Utility;

Image image; // the global Image we work on

Argument<int> arg_quality ("", "quality",
			   "quality setting used for writing compressed images\n\t\t"
			   "integer range 0-100, the default is 75",
			   0, 1, true, true);

Argument<std::string> arg_compression ("", "compress",
				       "compression method for writing images e.g. G3, G4, Zip, ...\n\t\t"
				       "depending on the output format, a reasonable setting by default",
				       0, 1, true, true);

Argument<std::string> arg_decompression ("", "decompress",
					 "decompression method for reading images e.g. thumb\n\t\t"
					 "depending on the input format, allowing to read partial data",
					 0, 1, true, true);

bool convert_input (const Argument<std::string>& arg)
{
  image.setRawData (0);

  std::string decompression = "";
  if (arg_decompression.Size())
    decompression = arg_decompression.Get();

  if (!ImageCodec::Read(arg.Get(), image, decompression)) {
    std::cerr << "Error reading input file." << std::endl;
    return false;
  }
  return true;
}

bool convert_output (const Argument<std::string>& arg)
{
  // we can check this way anymore as it might trigger on-the-fly
  // decoding
#if 0
  if (image.getRawData() == 0) {
    std::cerr << "No image available." << std::endl;
    return false;
  }
#endif
  
  int quality = 75;
  if (arg_quality.Size())
    quality = arg_quality.Get();
  std::string compression = "";
  if (arg_compression.Size())
    compression = arg_compression.Get();
  
  if (!ImageCodec::Write(arg.Get(), image, quality, compression)) {
    std::cerr << "Error writing output file." << std::endl;
    return false;
  }
  return true;
}

bool convert_split (const Argument<std::string>& arg)
{
  if (image.getRawData() == 0) {
    std::cerr << "No image available." << std::endl;
    return false;
  }
  
  Image split_image (image);
  
  // this is a bit ugly hacked for now
  split_image.h /= arg.count;
  if (split_image.h == 0) {
    std::cerr << "Resulting image size too small." << std::endl
	      << "The resulting slices must have at least a height of one pixel."
	      << std::endl;
    
    return false;
  }
  
  int quality = 70;
  if (arg_quality.Size())
    quality = arg_quality.Get();
  std::string compression = "";
  if (arg_compression.Size())
    compression = arg_compression.Get();

  int err = 0;
  for (int i = 0; i < arg.Size(); ++i)
    {
      std::cerr << "Writing file: " << arg.Get(i) << std::endl;
      split_image.setRawDataWithoutDelete
        (image.getRawData() + i * split_image.stride() * split_image.h);
      if (!ImageCodec::Write (arg.Get(i), split_image, quality, compression)) {
	err = 1;
	std::cerr << "Error writing output file." << std::endl;
      }
    }
  split_image.setRawDataWithoutDelete (0); // not deallocate in dtor
  
  return err == 0;
}

bool convert_colorspace (const Argument<std::string>& arg)
{
  return colorspace_by_name (image, arg.Get().c_str());
}

bool convert_normalize (const Argument<bool>& arg)
{
  normalize (image);
  return true;
}

bool convert_brightness (const Argument<double>& arg)
{
  double f = arg.Get();
  brightness_contrast_gamma (image, f, .0, 1.0);
  return true;
}

bool convert_contrast (const Argument<double>& arg)
{
  double f = arg.Get();
  brightness_contrast_gamma (image, .0, f, 1.0);
  return true;
}

bool convert_gamma (const Argument<double>& arg)
{
  double f = arg.Get();
  brightness_contrast_gamma (image, .0, .0, f);
  return true;
}

bool convert_blur (const Argument<double>& arg)
{
  double standard_deviation = arg.Get();
  GaussianBlur(image, standard_deviation);
  return true;
}

bool convert_scale (const Argument<double>& arg)
{
  double f = arg.Get();
  scale (image, f, f);
  return true;
}

bool convert_hue (const Argument<double>& arg)
{
  double f = arg.Get();
  hue_saturation_lightness (image, f, 0, 0);
  return true;
}

bool convert_saturation (const Argument<double>& arg)
{
  double f = arg.Get();
  hue_saturation_lightness (image, 0, f, 0);
  return true;
}

bool convert_lightness (const Argument<double>& arg)
{
  double f = arg.Get();
  hue_saturation_lightness (image, 0, 0, f);
  return true;
}

bool convert_nearest_scale (const Argument<double>& arg)
{
  double f = arg.Get();
  nearest_scale (image, f, f);
  return true;
}

bool convert_bilinear_scale (const Argument<double>& arg)
{
  double f = arg.Get();
  bilinear_scale (image, f, f);
  return true;
}

bool convert_bicubic_scale (const Argument<double>& arg)
{
  double f = arg.Get();
  bicubic_scale (image, f, f);
  return true;
}

bool convert_box_scale (const Argument<double>& arg)
{
  double f = arg.Get();
  box_scale (image, f, f);
  return true;
}

bool convert_ddt_scale (const Argument<double>& arg)
{
  double f = arg.Get();
  ddt_scale (image, f, f);
  return true;
}

bool convert_shear (const Argument<std::string>& arg)
{
  double xangle, yangle;;
  int n;
  // parse
  
  // TODO: pretty C++ parser
  if ((n = sscanf(arg.Get().c_str(), "%lfx%lf", &xangle, &yangle)))
    {
      if (n < 2)
	yangle = xangle;
    }
  
  std::cerr << "Shear: " << xangle << ", " << yangle << std::endl;
  
  shear (image, xangle, yangle);
  return true;
}

bool convert_flip (const Argument<bool>& arg)
{
  flipY (image);
  return true;
}

bool convert_flop (const Argument<bool>& arg)
{
  flipX (image);
  return true;
}

bool convert_rotate (const Argument<double>& arg)
{
  rotate (image, arg.Get(), background_color);
  return true;
}

bool convert_convolve (const Argument<double>& arg)
{
  double divisor = 0;
  const std::vector<double>& v = arg.Values ();
  int n = (int)sqrt(v.size());
  
  for (unsigned int i = 0; i < v.size(); ++i)
    divisor += v[i];
  
  if (divisor == 0)
    divisor = 1;
  
  convolution_matrix (image, &v[0], n, n, divisor);
  return true;
}

bool convert_dither_floyd_steinberg (const Argument<int>& arg)
{
  if (image.spp != 1 || image.bps != 8) {
    std::cerr << "Can only dither GRAY data right now." << std::endl;
    return false;
  }
  FloydSteinberg (image.getRawData(), image.w, image.h, arg.Get());
  return true;
}

bool convert_dither_riemersma (const Argument<int>& arg)
{
  if (image.spp != 1 || image.bps != 8) {
    std::cerr << "Can only dither GRAY data right now." << std::endl;
    return false;
  }
  Riemersma (image.getRawData(), image.w, image.h, arg.Get());
  return true;
}

bool convert_deskew (const Argument<int>& arg)
{
  deskew (image, arg.Get());
  return true;
}

bool convert_edge (const Argument<bool>& arg)
{
  matrix_type matrix[] = { -1.0, 0.0,  1.0,
                           -2.0, 0.0,  2.0,
                           -1.0, 0.0, -1.0 };

  convolution_matrix (image, matrix, 3, 3, (matrix_type)3.0);
  return true;
}

bool convert_resolution (const Argument<std::string>& arg)
{
  int xres, yres, n;
  // parse
  
  // TODO: pretty C++ parser
  if ((n = sscanf(arg.Get().c_str(), "%dx%d", &xres, &yres)))
    {
      if (n < 2)
	yres = xres;
      image.xres = xres;
      image.yres = yres;
      return true;
    }
  std::cerr << "Resolution '" << arg.Get() << "' could not be parsed." << std::endl;
  return false;
}

bool convert_size (const Argument<std::string>& arg)
{
  int w, h, n;
  // parse
  
  // TODO: pretty C++ parser
  if ((n = sscanf(arg.Get().c_str(), "%dx%d", &w, &h)) == 2)
    {
      image.w = w;
      image.h = h;
      image.setRawData (0);
      return true;
    }
  std::cerr << "Resolution '" << arg.Get() << "' could not be parsed." << std::endl;
  return false;
}

bool convert_crop (const Argument<std::string>& arg)
{
  int x,y, w, h, n;
  // parse
  
  // TODO: pretty C++ parser
  if ((n = sscanf(arg.Get().c_str(), "%d,%d,%d,%d", &x, &y, &w, &h)) == 4)
    {
      crop (image, x, y, w, h);
      return true;
    }
  std::cerr << "Crop '" << arg.Get() << "' could not be parsed." << std::endl;
  return false;
}

bool convert_fast_auto_crop (const Argument<bool>& arg)
{
  fastAutoCrop (image);
  return true;
}

bool convert_invert (const Argument<bool>& arg)
{
  invert (image);
  return true;
}

bool convert_deinterlace (const Argument<bool>& arg)
{
  deinterlace (image);
  return true;
}

/*
  #RGB                 (R,G,B are hex numbers, 4 bits each)
  #RRGGBB              (8 bits each)
  #RRRGGGBBB           (12 bits each)
  #RRRRGGGGBBBB        (16 bits each)
  
  TODO:

  #RGBA                (4 bits each)
  #RRGGBBOO            (8 bits each)
  #RRRGGGBBBOOO        (12 bits each)
  #RRRRGGGGBBBBOOOO    (16 bits each)
  
  X11, SVG standard colors
  name                 (identify -list color to see names)
  
  rgb(r,g,b)           0-255 for each of rgb
  rgba(r,g,b,a)        0-255 for each of rgb and 0-1 for alpha
  cmyk(c,m,y,k)        0-255 for each of cmyk
  cmyka(c,m,y,k,a)     0-255 for each of cmyk and 0-1 for alpha
  
  hsl(0, 100%, 50%) }

*/

static struct {
  const char* name;
  const char* color;
} named_colors[]  = {
  // CSS2
  { "white",   "#ffffff" },
  { "yellow",  "#ffff00" },
  { "orange",  "#ffA500" },
  { "red",     "#ff0000" },
  { "fuchsia", "#ff00ff" },
  { "silver",  "#c0c0c0" },
  { "gray",    "#808080" },
  { "olive",   "#808000" },
  { "purple",  "#800080" },
  { "maroon",  "#800000" },
  { "aqua",    "#00ffff" },
  { "lime",    "#00ff00" },
  { "teal",    "#008080" },
  { "green",   "#008000" },
  { "blue",    "#0000ff" },
  { "navy",    "#000080" },
  { "black",   "#000000" },
};

static bool parse_color (Image::iterator& it, const char* color)
{
  unsigned int r, g, b;
  int strl = strlen(color);
  
  std::cerr << "parse_color: " << color << std::endl;
  
  if (strl == 4 && sscanf(color, "#%1x%1x%1x", &r, &g, &b) == 3)
    {
      double _r, _g, _b;
      _r = 1. / 0xf * r;
      _g = 1. / 0xf * g;
      _b = 1. / 0xf * b;
      it.setRGB(_r, _g, _b);
      return true;
    }
  else if (strl == 7 && sscanf(color, "#%2x%2x%2x", &r, &g, &b) == 3)
    {
      double _r, _g, _b;
      _r = 1. / 0xff * r;
      _g = 1. / 0xff * g;
      _b = 1. / 0xff * b;
      it.setRGB(_r, _g, _b);
      return true;
    }
  else if (strl == 10 && sscanf(color, "#%3x%3x%3x", &r, &g, &b) == 3)
    {
      double _r, _g, _b;
      _r = 1. / 0xfff * r;
      _g = 1. / 0xfff * g;
      _b = 1. / 0xfff * b;
      it.setRGB(_r, _g, _b);
      return true;
    }
  else if (strl == 13 && sscanf(color, "#%4x%4x%4x", &r, &g, &b) == 3)
    {
      double _r, _g, _b;
      _r = 1. / 0xffff * r;
      _g = 1. / 0xffff * g;
      _b = 1. / 0xffff * b;
      it.setRGB(_r, _g, _b);
      return true;
    }
  else for (unsigned i = 0; i < ARRAY_SIZE(named_colors); ++i)
    {
      if (strcmp(color, named_colors[i].name) == 0)
	return parse_color(it, named_colors[i].color);
    }
  return false;
}

bool convert_background (const Argument<std::string>& arg)
{
  if (!parse_color(background_color, arg.Get().c_str())) {
    std::cerr << "Error parsing color: '" << arg.Get() << "'" << std::endl;
    return false;
  }
  return true;
}

bool convert_foreground (const Argument<std::string>& arg)
{
  if (!parse_color(foreground_color, arg.Get().c_str())) {
    std::cerr << "Error parsing color: '" << arg.Get() << "'" << std::endl;
    return false;
  }
  return true;
}

bool convert_line (const Argument<std::string>& arg)
{
  unsigned int x1, y1, x2, y2;
  
  if (sscanf(arg.Get().c_str(), "%d,%d,%d,%d", &x1, &y1, &x2, &y2) == 4)
    {
      Path path;
      path.moveTo (x1, y1);
      path.addLineTo (x2, y2);
      
      double r = 0, g = 0, b = 0;
      foreground_color.getRGB (r, g, b);
      path.setFillColor (r, g, b);
      path.draw (image);
      return true; 
    }
  
  std::cerr << "Error parsing line: '" << arg.Get() << "'" << std::endl;
  return false;
}

#if WITHFREETYPE == 1

bool convert_text (const Argument<std::string>& arg)
{
  unsigned int x1, y1;
  double height;
  char text[512];
  
  if (sscanf(arg.Get().c_str(), "%d,%d,%lf,%[ a-zA-Z0-9+*/_-]",
	     &x1, &y1, &height, text) == 4)
    {
      std::cerr << x1 << ", " << y1 << ", " << height
		<< ", " << text << std::endl;
      Path path;
      path.moveTo (x1, y1);
      
      double r = 0, g = 0, b = 0;
      foreground_color.getRGB (r, g, b);
      path.setFillColor (r, g, b);
      path.drawText (image, text, height);

      return true; 
    }
  
  std::cerr << "Error parsing line: '" << arg.Get() << "'" << std::endl;
  return false;
}

#endif

int main (int argc, char* argv[])
{
  ArgumentList arglist;
  background_color.type = Image::RGB8;
  background_color.setL (255);
  foreground_color.type = Image::RGB8;
  foreground_color.setL (0);
  
  // setup the argument list
  Argument<bool> arg_help ("h", "help",
			   "display this help text and exit");
  arglist.Add (&arg_help);
  
  Argument<std::string> arg_input ("i", "input",
				   "input file or '-' for stdin, optionally prefixed with format:"
				   "\n\t\te.g: jpg:- or raw:rgb8-dump",
                                   0, 1, true, true);
  arg_input.Bind (convert_input);
  arglist.Add (&arg_input);
  
  Argument<std::string> arg_output ("o", "output",
				    "output file or '-' for stdout, optinally prefix with format:"
				    "\n\t\te.g. jpg:- or raw:rgb8-dump",
				    0, 1, true, true);
  arg_output.Bind (convert_output);
  arglist.Add (&arg_output);
  
  // global
  arglist.Add (&arg_quality);
  arglist.Add (&arg_compression);
  arglist.Add (&arg_decompression);
  
  Argument<std::string> arg_split ("", "split",
				   "filenames to save the images split in Y-direction into n parts",
				   0, 999, true, true);
  arg_split.Bind (convert_split);
  arglist.Add (&arg_split);
  
  Argument<std::string> arg_colorspace ("", "colorspace",
					"convert image colorspace (BW, BILEVEL, GRAY, GRAY1, GRAY2, GRAY4,\n\t\tRGB, YUV, CYMK)",
					0, 1, true, true);
  arg_colorspace.Bind (convert_colorspace);
  arglist.Add (&arg_colorspace);

  Argument<bool> arg_normalize ("", "normalize",
				"transform the image to span the full color range",
				0, 0, true, true);
  arg_normalize.Bind (convert_normalize);
  arglist.Add (&arg_normalize);

  Argument<double> arg_brightness ("", "brightness",
				   "change image brightness",
				   0.0, 0, 1, true, true);
  arg_brightness.Bind (convert_brightness);
  arglist.Add (&arg_brightness);

  Argument<double> arg_contrast ("", "contrast",
				 "change image contrast",
				 0.0, 0, 1, true, true);
  arg_contrast.Bind (convert_contrast);
  arglist.Add (&arg_contrast);

  Argument<double> arg_gamma ("", "gamma",
				 "change image gamma",
				 0.0, 0, 1, true, true);
  arg_gamma.Bind (convert_gamma);
  arglist.Add (&arg_gamma);

  Argument<double> arg_hue ("", "hue",
				 "change image hue",
				 0.0, 0, 1, true, true);
  arg_hue.Bind (convert_hue);
  arglist.Add (&arg_hue);

  Argument<double> arg_saturation ("", "saturation",
				   "change image saturation",
				   0.0, 0, 1, true, true);
  arg_saturation.Bind (convert_saturation);
  arglist.Add (&arg_saturation);

  Argument<double> arg_lightness ("", "lightness",
				  "change image lightness",
				  0.0, 0, 1, true, true);
  arg_lightness.Bind (convert_lightness);
  arglist.Add (&arg_lightness);

  Argument<double> arg_blur ("", "blur",
				 "gaussian blur",
				 0.0, 0, 1, true, true);
  arg_blur.Bind (convert_blur);
  arglist.Add (&arg_blur);

  
  Argument<double> arg_scale ("", "scale",
			      "scale image data using a method suitable for specified factor",
			      0.0, 0, 1, true, true);
  arg_scale.Bind (convert_scale);
  arglist.Add (&arg_scale);
  
  Argument<double> arg_nearest_scale ("", "nearest-scale",
				   "scale image data to nearest neighbour",
				   0.0, 0, 1, true, true);
  arg_nearest_scale.Bind (convert_nearest_scale);
  arglist.Add (&arg_nearest_scale);

  Argument<double> arg_bilinear_scale ("", "bilinear-scale",
				       "scale image data with bi-linear filter",
				       0.0, 0, 1, true, true);
  arg_bilinear_scale.Bind (convert_bilinear_scale);
  arglist.Add (&arg_bilinear_scale);

  Argument<double> arg_bicubic_scale ("", "bicubic-scale",
				      "scale image data with bi-cubic filter",
				      0.0, 0, 1, true, true);
  arg_bicubic_scale.Bind (convert_bicubic_scale);
  arglist.Add (&arg_bicubic_scale);

  Argument<double> arg_ddt_scale ("", "ddt-scale",
				      "scale image data with data dependant triangulation",
				      0.0, 0, 1, true, true);
  arg_ddt_scale.Bind (convert_ddt_scale);
  arglist.Add (&arg_ddt_scale);
  
  Argument<double> arg_box_scale ("", "box-scale",
				   "(down)scale image data with box filter",
				  0.0, 0, 1, true, true);
  arg_box_scale.Bind (convert_box_scale);
  arglist.Add (&arg_box_scale);

  Argument<double> arg_rotate ("", "rotate",
			       "rotation angle",
			       0, 1, true, true);
  arg_rotate.Bind (convert_rotate);
  arglist.Add (&arg_rotate);

  Argument<double> arg_convolve ("", "convolve",
			       "convolution matrix",
			       0, 9999, true, true);
  arg_convolve.Bind (convert_convolve);
  arglist.Add (&arg_convolve);

  Argument<bool> arg_flip ("", "flip",
			   "flip the image vertically",
			   0, 0, true, true);
  arg_flip.Bind (convert_flip);
  arglist.Add (&arg_flip);

  Argument<bool> arg_flop ("", "flop",
			   "flip the image horizontally",
			   0, 0, true, true);
  arg_flop.Bind (convert_flop);
  arglist.Add (&arg_flop);

  Argument<int> arg_floyd ("", "floyd-steinberg",
			   "Floyd Steinberg dithering using n shades",
			   0, 1, true, true);
  arg_floyd.Bind (convert_dither_floyd_steinberg);
  arglist.Add (&arg_floyd);
  
  Argument<int> arg_riemersma ("", "riemersma",
			       "Riemersma dithering using n shades",
			       0, 1, true, true);
  arg_riemersma.Bind (convert_dither_riemersma);
  arglist.Add (&arg_riemersma);


  Argument<bool> arg_edge ("", "edge",
                           "edge detect filter",
			   0, 0, true, true);
  arg_edge.Bind (convert_edge);
  arglist.Add (&arg_edge);
  
  Argument<int> arg_deskew ("", "deskew",
			     "deskew digitalized paper",
			     0, 1, true, true);
  arg_deskew.Bind (convert_deskew);
  arglist.Add (&arg_deskew);

  Argument<std::string> arg_resolution ("", "resolution",
					"set meta data resolution in dpi to x[xy] e.g. 200 or 200x400",
					0, 1, true, true);
  arg_resolution.Bind (convert_resolution);
  arglist.Add (&arg_resolution);

  Argument<std::string> arg_size ("", "size",
			      "width and height of raw images whose dimensions are unknown",
			      0, 1, true, true);
  arg_size.Bind (convert_size);
  arglist.Add (&arg_size);

  Argument<std::string> arg_crop ("", "crop",
			      "crop an area out of an image: x,y,w,h",
			      0, 1, true, true);
  arg_crop.Bind (convert_crop);
  arglist.Add (&arg_crop);

  Argument<bool> arg_fast_auto_crop ("", "fast-auto-crop",
				     "fast auto crop",
				     0, 0, true, true);
  arg_fast_auto_crop.Bind (convert_fast_auto_crop);
  arglist.Add (&arg_fast_auto_crop);

  Argument<bool> arg_invert ("", "negate",
                             "negates the image",
                               0, 0, true, true);
  arg_invert.Bind (convert_invert);
  arglist.Add (&arg_invert);

  Argument<bool> arg_deinterlace ("", "deinterlace",
				  "shuffleg every 2nd line",
				  0, 0, true, true);
  arg_deinterlace.Bind (convert_deinterlace);
  arglist.Add (&arg_deinterlace);

  Argument<std::string> arg_background ("", "background",
					"background color used for operations",
					0, 1, true, true);
  arg_background.Bind (convert_background);
  arglist.Add (&arg_background);
  
  Argument<std::string> arg_foreground ("", "foreground",
					"foreground color used for operations",
					0, 1, true, true);
  arg_foreground.Bind (convert_foreground);
  arglist.Add (&arg_foreground);
 
  Argument<std::string> arg_line ("", "line",
                                  "draw a line: x1, y1, x2, y2",
                                   0, 1, true, true);
  arg_line.Bind (convert_line);
  arglist.Add (&arg_line);

#if WITHFREETYPE == 1
  Argument<std::string> arg_text ("", "text",
                                  "draw text: x1, y1, height, text",
				  0, 1, true, true);
  arg_text.Bind (convert_text);
  arglist.Add (&arg_text);
#endif
  
  // parse the specified argument list - and maybe output the Usage
  if (!arglist.Read (argc, argv))
    return 1;

  // print the usage if no argument was given or help requested
  if (argc == 1 || arg_help.Get() == true)
    {
      std::cerr << "ExactImage converter, version " VERSION << std::endl
		<< "Copyright (C) 2005 - 2008 René Rebe, ExactCODE" << std::endl
		<< "Copyright (C) 2005, 2008 Archivista" << std::endl
		<< "Usage:" << std::endl;
      
      arglist.Usage (std::cerr);
      return 1;
    }
  
  
  // all is done inside the argument callback functions
  return 0;
}
