/*
 * Vector element rasterization, via Agg.
 * Copyright (C) 2007 Susanne Klaus, René Rebe ExactCODE GmbH
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

#ifndef VECTORIAL_HH
#define VECTORIAL_HH

#include "Image.hh"
#include <vector>

#include "agg_math_stroke.h"
#include "agg_path_storage.h"

class drawStyle
{
public:
  drawStyle ()
    : width (1) {
  }
  
  double width;
  std::vector <double> dash;
};

class Path
{
public:
  Path ();
  ~Path ();

  void moveTo (double x, double y);
  void addLineTo (double x, double y);
  
  void addArcTo (double rx, double ry,  double angle,
		 double x, double y);

  void addArc (double rx, double ry,  double angle,
	       double dx, double dy);
  
  /* TODO:
     - addRect
     - addEllipse
  */
  
  
  void addCurveTo (double, double, double, double);
  // or explicitly differentiate by naming this one QuadCurve ???
  void addCurveTo (double, double, double, double, double, double);

  void end ();
  void close ();
  
  void setFillColor (double r, double g, double b, double a = 1.0);
  void setLineWidth (double width);
  void setLineDash (double offset, const std::vector<double>& dashes);
  void setLineDash (double offset, const double* dashes, int n);
  
  typedef agg::line_cap_e line_cap_t;
  void setLineCap (line_cap_t cap);
  
  typedef agg::line_join_e line_join_t;
  void setLineJoin (line_join_t join);
  
  /* TODO:
     - clip
     - control anti-aliasing
     (- gradients)
  */
  
  enum filling_rule_t
    {
      fill_non_zero = agg::fill_non_zero,
      fill_even_odd = agg::fill_even_odd,
      fill_none = 0xff
    };

  void draw (Image& image, filling_rule_t fill = fill_none);
  
protected:
  agg::path_storage path;
  
  // quick hack ("for now")
  double r, g, b, a, line_width, dashes_start_offset;
  std::vector <double> dashes;
  
  line_cap_t line_cap;
  line_join_t line_join;
};

void drawLine(Image& img, double x, double y, double x2, double y2,
	      const Image::iterator& color, const drawStyle& style);

void drawRectangle(Image& img, double x, double y, double x2, double y2,
		  const Image::iterator& color, const drawStyle& style);

void drawText(Image& image, double x, double y, char* text, double height,
	      const Image::iterator& color);

// new, final (?) API

#if 0
void beginPath(context);
void moveTo (context,x,y);
void addLineTo (context,x,y);
void addCurveTo (context,c1x, c1y, c2x, c2y, x, y);
void addQuadCurveTo ();
void closePath (context);
void setLineWidth (context, width);
void drawPath (context, path-styling?);

// joins
// line caps
// line dashing
// fill
// clipping
// anti-aliasing

// utilties
// rect
// arc
// roundedRect


#endif

#endif
