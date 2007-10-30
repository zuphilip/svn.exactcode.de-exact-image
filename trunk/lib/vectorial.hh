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

class drawStyle
{
public:
  drawStyle ()
    : width (1), caps (BUTT), join (DEFAULT) {
  }
  
  double width;
  std::vector <double> dash;
  
  enum {
    BUTT,
    SQUARE,
    ROUND
  } caps;

  enum {
    DEFAULT,
    /* ACCURATE, MITER; ROUND, BEVEL, ... */
  } join;
  
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
