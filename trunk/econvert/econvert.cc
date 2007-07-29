/*
 * The ExactImage library's convert compatible command line frontend.
 * Copyright (C) 2006, 2007 René Rebe
 * Copyright (C) 2006 Archivista
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

#include "config.h"

#include "ArgumentList.hh"

#include "Image.hh"
#include "Codecs.hh"

#include "Colorspace.hh"

#include "scale.hh"
#include "crop.hh"
#include "rotate.hh"
#include "Matrix.hh"
#include "riemersma.h"
#include "floyd-steinberg.h"
#include "vectorial.hh"

/* Let's reuse some parts of the official, stable API to avoid
 * duplicating code.
 *
 * This also includes the foreground/background color and vector
 * drawing style.
 */

#include "api/api.cc"

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

bool convert_input (const Argument<std::string>& arg)
{
  image.setRawData (0);

  if (!ImageCodec::Read(arg.Get(), image)) {
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
        (image.getRawData() + i * split_image.Stride() * split_image.h);
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
  return imageConvertColorspace (&image, arg.Get().c_str());
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
  const std::vector<double>& v = arg.Values ();
  int n = sqrt(v.size());
  convolution_matrix (image, &v[0], n, n, 1.0);
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

bool convert_edge (const Argument<bool>& arg)
{
  matrix_type matrix[] = { -1.0, 0.0,  1.0,
                           -2.0, 0.0,  2.0,
                           -1.0, 0.0, -1.0 };

  convolution_matrix (image, matrix, 3, 3, (matrix_type)3.0);
  return true;
}

bool convert_descew (const Argument<bool>& arg)
{
  using std::cout;
  using std::endl;
  
  uint8_t* new_data = (uint8_t*) malloc (image.Stride() * image.h);

#define pix(d,x,y) d[image.w*(y)+x]
  const int thr = 8;

  uint8_t* data = image.getRawData();
 
  // accumulate deltas
  for (int x = 0; x < image.w-1; ++x) {
    for (int y = 0; y < image.h-1; ++y)
      {
	int i = std::abs ((int)pix(data,x,y) - pix(data,x+1,y));
	i += std::abs ((int)pix(data,x,y) - pix(data,x,y+1));
	if (i > thr)
	  pix(new_data,x,y) += i*2;
      }
  }

  // denoise
  for (int i = 0; i < 2; ++i) {
    for (int x = 2; x < image.w-2; ++x) {
      for (int y = 2; y < image.h-2; ++y) {
	if (pix(new_data,x,y)) {
	  int z = 0;
	  
	  if (pix(new_data,x-2,y))
	    ++z;
	  if (pix(new_data,x-1,y))
	    ++z;
	  if (pix(new_data,x+1,y))
	    ++z;
	  if (pix(new_data,x+2,y))
	    ++z;
	  
	  if (pix(new_data,x,y-2))
	    ++z;
	  if (pix(new_data,x,y-1))
	    ++z;
	  if (pix(new_data,x,y+1))
	    ++z;
	  if (pix(new_data,x,y+2))
	    ++z;
	  
	  if (z < 4)
	    pix(new_data,x,y) = 0;
	}
      }
    }
  }
  for (int x = 0; x < image.w; ++x) {
    pix(new_data,x,0) = pix(new_data,x,1) = 0;
    pix(new_data,x,image.h-2) = pix(new_data,x,image.h-1) = 0;
  }
  
  for (int y = 0; y < image.h; ++y) {
    pix(new_data,0,y) = pix(new_data,1,y) = 0;
    pix(new_data,image.w-2,y) = pix(new_data,image.w-1,y) = 0;
  }
   
  
  // analyze phase, mark first contrast change coordinates
  int *top, *bottom, *left, *right;
  top = new int[image.w];
  bottom = new int[image.w];
  left = new int[image.h];
  right = new int[image.h];

  // area defines
  const int top_border = std::min (32, image.h/5);
  const int left_border = image.w/2;
  const int right_border = image.w/2;
  const int bottom_border = image.h/4;

  // top/bottom
  for (int x = 1; x < image.w-1; ++x) {
    top[x] = 0;
    //cout << x << ":";

    for (int y = 0; y < top_border; ++y)
      {
        if (pix(new_data,x,y) > 0 && (pix(new_data,x-1,y) || pix(new_data,x+1,y) ) > 0)
	  {
             //cout << " top: " << y;
             top[x] = y;
             break;
          }
      }
    bottom[x] = 0;
    for (int y = image.h-1; y >= bottom_border; --y)
      {
        if (pix(new_data,x,y) > 0 && (pix(new_data,x-1,y) || pix(new_data,x+1,y) ) > 0)
	  {
             //cout << " bottom: " << y;
             bottom[x] = y;
             break;
          }
      }
    //cout << endl;
  }


  // sides
  for (int y = 1; y < image.h-1; ++y) {
    left[y] = 0;
    //cout << y << ":";
    for (int x = 0; x < left_border; ++x)
      {
        if (pix(new_data,x,y) > 0 && (pix(new_data,x,y-1) || pix(new_data,x,y+1) ) > 0)
	  {
             //cout << " left: " << x;
             left[y] = x;
             break;
          }
      }
    right[y] = 0;
    for (int x = image.w-1; x >= right_border; --x)
      {
        if (pix(new_data,x,y) > 0 && (pix(new_data,x,y-1) || pix(new_data,x,y+1) ) > 0)
	  {
             //cout << " right: " << x;
             right[y] = x;
             break;
          }
      }
    //cout << endl;
  }

  image.setRawData (new_data);
  data = image.getRawData();
       

  // visualize
  
#define mark(x,y) {data[ image.Stride()* (y) + (x) ] = 0xff;}

  int* hori_histogramm = new int [image.w];
  int* vert_histogramm = new int [image.h];
  
  for (int x = 1; x < image.w-1; ++x) {
	if (top[x]) {
	  vert_histogramm[top[x]]++;
          mark (x, top[x]);
	}
	if (bottom[x]) {
	  vert_histogramm[bottom[x]]++;
          mark (x, bottom[x]);
	}
  }

  for (int y = 1; y < image.h-1; ++y) {
	if (left[y]) {
	  hori_histogramm[left[y]]++;
          mark (left[y], y);
	}
	if (right[y]) {
	  hori_histogramm[right[y]]++;
          mark (right[y], y);
	}
  }
  
#define mark2(x,y,v) {data[ image.Stride()* (y) + (x) ] = v; }

#if 0  
  for (int x = 2; x < image.w-2; ++x) {
    mark2(x,image.h/2-2,0xff);
    mark2(x,image.h/2-1,hori_histogramm[x]);
    mark2(x,image.h/2,  hori_histogramm[x]);
    mark2(x,image.h/2+1,hori_histogramm[x]);
    mark2(x,image.h/2+2,0xff);
  }
  
  for (int y = 2; y < image.h-2; ++y) {
    mark2(image.w/2-2,y,0xff);
    mark2(image.w/2-1,y,vert_histogramm[y]);
    mark2(image.w/2,  y,vert_histogramm[y]);
    mark2(image.w/2+1,y,vert_histogramm[y]);
    mark2(image.w/2+2,y,0xff);
  }
#endif
  
  // find first histogramm spikes
  const int thr2 = 48; // min pixel count we want to trace the line
  
  int spike;
  std::vector<std::pair<int,int> > points;
  std::map<int,int> angles;
  
  double left_angle, right_angle, bottom_angle;
  
  const int mindist = 32;
  
  points.clear();
  spike = 0;
  for (int x = 1; x < left_border; ++x) {
    int z = hori_histogramm[x] +
      hori_histogramm[x+1] +
      hori_histogramm[x+2] +
      hori_histogramm[x+3];
    
    if (spike == 0 && z > thr2)
      spike = x;
    
    if (spike != 0 && z < thr2) {
      cout << "left spike: " << spike << " - " << x << endl;
      
      // collect all points in the given area
      for (int y = 1; y < image.h-1; ++y) {
	for (int xx = spike; xx < x; ++xx) {
	  if (pix(new_data,xx,y) == 0xff) {
	    points.push_back (std::pair<int,int> (xx,y));
	    // y += mindist; // skip some points to reduce cpu load
	    break;
	  }
	}
      }
      break;
    }
  }
  
  // ---
   cout << "size: " << points.size() << endl;
  angles.clear();
  for (std::vector<std::pair<int,int> >::iterator it = points.begin(); it != points.end(); ++it)
    {
      std::vector<std::pair<int,int> >::iterator it2 = it;
      for (++it2; it2 != points.end(); ++it2)
	{
	  std::pair<int,int> p1, p2;
	  if (it->second < it2->second) {
	    p1 = *it; p2 = *it2;
	  }
	  else {
	    p1 = *it2; p2 = *it;
	  }
	  
	  double dx = p2.first - p1.first;
	  double dy = p2.second - p1.second;
	  if (dy > mindist) {
	    int angle = (int)(-atan (dx/dy) / M_PI * 180 * 100);
	    angles[angle] ++;
	  }
	}
    }
  
  // most accuring angle:
  {
    std::map<int,int>::iterator high = angles.begin();
    for (std::map<int,int>::iterator it = angles.begin();
	 it != angles.end(); ++it) {
      //cout << "1 " << it->second << " " << it->first << endl;
      if (it->first != 0 && it->second > high->second) {
	high = it;
      }
    }
    
    left_angle = (double)high->first / 100;
    cout << "Left angle: " << left_angle << endl;
  }

  points.clear();
  spike = 0;
  for (int x = image.w-1; x > right_border; --x) {
    int z = hori_histogramm[x] +
      hori_histogramm[x-1] +
      hori_histogramm[x-2] +
      hori_histogramm[x-3];
    
    if (spike == 0 && z > thr2)
      spike = x;
    
    if (spike != 0 && z < thr2) {
      cout << "right spike: " << spike << " - " << x << endl;
      
      // collect all points in the given area
      for (int y = 1; y < image.h-1; ++y) {
	for (int xx = spike; xx > x; --xx) {
	  if (pix(new_data,xx,y) == 0xff) {
	    points.push_back (std::pair<int,int> (xx,y));
	    // y += mindist; // skip some points to reduce cpu load
	    break;
	  }
	}
      }
      break;
    }
  }

  cout << "size: " << points.size() << endl;
  angles.clear();
  for (std::vector<std::pair<int,int> >::iterator it = points.begin(); it != points.end(); ++it)
    {
      std::vector<std::pair<int,int> >::iterator it2 = it;
      for (++it2; it2 != points.end(); ++it2)
	{
	  std::pair<int,int> p1, p2;
	  if (it->second < it2->second) {
	    p1 = *it; p2 = *it2;
	  }
	  else {
	    p1 = *it2; p2 = *it;
	  }
	  
	  double dx = p2.first - p1.first;
	  double dy = p2.second - p1.second;
	  if (dy > mindist) {
	    int angle = (int)(-atan (dx/dy) / M_PI * 180 * 100);
	    angles[angle] ++;
	  }
	}
    }
  
  // most accuring angle:
  {
    std::map<int,int>::iterator high = angles.begin();
    for (std::map<int,int>::iterator it = angles.begin();
	 it != angles.end(); ++it) {
      //cout << "2 " << it->second << " " << it->first << endl;
      if (it->first != 0 && it->second > high->second) {
	high = it;
      }
    }
    right_angle = (double)high->first / 100;
    cout << "Right angle: " << right_angle << endl;
  }
  
  // ----------

  points.clear();
  spike = 0;
  for (int y = image.h-1; y > bottom_border; --y) {
    int z = vert_histogramm[y] +
      vert_histogramm[y-1] +
      vert_histogramm[y-2] +
      vert_histogramm[y-3];
    
    if (spike == 0 && z > thr2)
      spike = y;
    
    if (spike != 0 && z < thr2) {
      cout << "rbottom spike: " << spike << " - " << y << endl;
      
      // collect all points in the given area
      for (int x = 1; x < image.w-1; ++x) {
	for (int yy = spike; yy > y; --yy) {
	  if (pix(new_data,x,yy) == 0xff) {
	    points.push_back (std::pair<int,int> (x,yy));
	    // y += mindist; // skip some points to reduce cpu load
	    break;
	  }
	}
      }
      break;
    }
  }
  
  cout << "size: " << points.size() << endl;
  angles.clear();
  for (std::vector<std::pair<int,int> >::iterator it = points.begin(); it != points.end(); ++it)
    {
      std::vector<std::pair<int,int> >::iterator it2 = it;
      for (++it2; it2 != points.end(); ++it2)
	{
	  std::pair<int,int> p1, p2;
	  if (it->first < it2->first) {
	    p1 = *it; p2 = *it2;
	  }
	  else {
	    p1 = *it2; p2 = *it;
	  }
	  
	  double dx = p2.first - p1.first;
	  double dy = p2.second - p1.second;
	  
	  if (dx > mindist) {
	    int angle = (int)(atan (dy/dx) / M_PI * 180 * 100);
	    angles[angle] ++;
	  }
	}
    }
  
  // most accuring angle:
  {
    std::map<int,int>::iterator high = angles.begin();
    for (std::map<int,int>::iterator it = angles.begin();
	 it != angles.end(); ++it) {
      //cout << "3 " << it->second << " " << it->first << endl;
      if (it->first !=0 && it->second > high->second) {
	high = it;
      }
    }
    
    bottom_angle = (double)high->first / 100;
    cout << "Bottom angle: " << bottom_angle << endl;
  }
  
  delete[] top;
  delete[] bottom;
  delete[] left;
  delete[] right;
 
  double angle = .0;
  int n = 0;
  
  if (left_angle != .0) {
    angle -= left_angle; ++n;
  }

  if (right_angle != .0) {
    angle -= right_angle; ++n;
  }
  
  if (bottom_angle != .0) {
    angle -= bottom_angle; ++n;
  }
  angle /= n;
  
  cout << "rotating: " << angle << endl;
  rotate (image, angle, background_color);
  
  return true;
}

bool convert_descew2 (const Argument<bool>& arg)
{
  using std::cout;
  using std::endl;
  
  // find identical vertical pixels
  Image::iterator it1 = image.begin();
  Image::iterator it2 = image.begin();
  Image::iterator it3 = image.begin();
  Image::iterator it4 = image.begin();
  Image::iterator it5 = image.begin();
  it2 = it2.at (0, 1);
  it3 = it3.at (0, 2);
  it4 = it4.at (0, 3);
  it5 = it5.at (0, 4);
  
  for (int y = 0; y < image.h-4; ++y) {
    for (int x = 0; x < image.w; ++x)
      {
#define get(x,r,g,b) {				\
	  uint16_t _r, _g, _b;			\
	  *x;					\
	  x.getRGB (&_r, &_g, &_b);		\
	  r = _r; g = _g; b = _b;		\
	}
	
	int r1, g1, b1;
	get (it1,r1,g1,b1);
	
	int r2, g2, b2;
	get (it2,r2,g2,b2);
	
	int r3, g3, b3;
	get (it3,r3,g3,b3);

	int r4, g4, b4;
	get (it4,r4,g4,b4);

	int r5, g5, b5;
	get (it5,r5,g5,b5);
	
	const int n = 2;
	
	if (
	    r2 >= r1-n && r2 <= r1+n &&
	    g2 >= g1-n && g2 <= g1+n &&
	    b2 >= b1-n && b2 <= b1+n &&
	    
	    r2 >= r3-n && r2 <= r3+n &&
	    g2 >= g3-n && g2 <= g3+n &&
	    b2 >= b3-n && b2 <= b3+n &&

	    r2 >= r4-n && r2 <= r4+n &&
	    g2 >= g4-n && g2 <= g4+n &&
	    b2 >= b4-n && b2 <= b4+n &&

	    r2 >= r5-n && r2 <= r5+n &&
	    g2 >= g5-n && g2 <= g5+n &&
	    b2 >= b5-n && b2 <= b5+n &&
	    
	    true
	    )
	  it2.setRGB ((uint16_t)255, (uint16_t)255, (uint16_t)0), it2.set (it2);
	
	++it1; ++it2; ++it3; ++it4; ++it5;
      }
  }
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

bool convert_background (const Argument<std::string>& arg)
{
  // parse
  /*
    name                 (identify -list color to see names)
    #RGB                 (R,G,B are hex numbers, 4 bits each)
    #RRGGBB              (8 bits each)
    #RRRGGGBBB           (12 bits each)
    #RRRRGGGGBBBB        (16 bits each)
    #RGBA                (4 bits each)
    #RRGGBBOO            (8 bits each)
    #RRRGGGBBBOOO        (12 bits each)
    #RRRRGGGGBBBBOOOO    (16 bits each)
    rgb(r,g,b)           0-255 for each of rgb
    rgba(r,g,b,a)        0-255 for each of rgb and 0-1 for alpha
    cmyk(c,m,y,k)        0-255 for each of cmyk
    cmyka(c,m,y,k,a)     0-255 for each of cmyk and 0-1 for alpha
  */
  
  std::string a = arg.Get();
  
  // TODO: pretty C++ parser
  if (a.size() && a[0] == '#')
    {
      
      return true;
    }
  
  std::cerr << "Error parsing color: '" << a << "'" << std::endl;
  return false;
}

bool convert_line (const Argument<std::string>& arg)
{
  unsigned int x1, y1, x2, y2, n;
  
  if ((n = sscanf(arg.Get().c_str(), "%d,%d,%d,%d", &x1, &y1, &x2, &y2)) == 4)
    {
      drawLine(image, x1, y1, x2, y2, foreground_color, style);
      return true; 
    }
  
  std::cerr << "Error parsing line: '" << arg.Get() << "'" << std::endl;
  return false;
}

int main (int argc, char* argv[])
{
  ArgumentList arglist;
  background_color.type = Image::RGB8;
  background_color.setL (255);
  foreground_color.type = Image::RGB8;
  foreground_color.setL (127);
  
  // setup the argument list
  Argument<bool> arg_help ("", "help",
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
  
  Argument<bool> arg_descew ("", "descew",
			     "descew digitalized paper",
			     0, 0, true, true);
  arg_descew.Bind (convert_descew);
  arglist.Add (&arg_descew);

  Argument<bool> arg_descew2 ("", "descew2",
			     "descew digitalized paper try2",
			     0, 0, true, true);
  arg_descew2.Bind (convert_descew2);
  arglist.Add (&arg_descew2);
  
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


  Argument<std::string> arg_background ("", "background",
					"background color used for operations",
					0, 1, true, true);
  arg_background.Bind (convert_background);
  arglist.Add (&arg_background);
 
  Argument<std::string> arg_line ("", "line",
                                  "draw a line: x1, y1, x2, y2",
                                   0, 1, true, true);
  arg_line.Bind (convert_line);
  arglist.Add (&arg_line);

 
  // parse the specified argument list - and maybe output the Usage
  if (!arglist.Read (argc, argv))
    return 1;

  // print the usage if no argument was given or help requested
  if (argc == 1 || arg_help.Get() == true)
    {
      std::cerr << "Exact image converter including a variety of fast algorithms (econvert)."
		<< std::endl << "Version " VERSION
                <<  " - Copyright (C) 2005, 2006 by René Rebe and Archivista" << std::endl
                << "Usage:" << std::endl;
      
      arglist.Usage (std::cerr);
      return 1;
    }
  
  
  // all is done inside the argument callback functions
  return 0;
}
