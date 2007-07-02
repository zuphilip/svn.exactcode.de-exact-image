/*
 * Image Line 
 * Copyright (C) 2007 Valentin Ziegler and René Rebe
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

#include "line.hh"
#include <iostream>

// dirty hack to draw a horizontal/vertical red line over the image
void Line::drawLine(Image& img, unsigned int a_start, unsigned int a_end, unsigned int b, bool horizontal)
{
  unsigned int width=(unsigned int) img.w;
  Image::iterator red=img.begin();
  red.setRGB(255, 0, 0);

  unsigned int line=0;
  unsigned int row=0;
  unsigned int max=(horizontal) ? b : a_end;

  Image::iterator i=img.begin();
  Image::iterator end=img.end();

  //just for testing
  std::cout << a_start << a_end << b << horizontal << std::endl;

  for (; i!=end && line <= max; ++i) {
    if (horizontal) {
      if (line==b && row >= a_start && row <= a_end)
	i.set(red);
    } else {
      if (row==b && line >= a_start && line <= a_end)
	i.set(red);
    }
    if (++row == width) {
      line++;
      row=0;
    }
  }
}

void Line::drawHLine(Image& img, unsigned int x_start, unsigned int x_end, unsigned int y)
{
  drawLine(img, x_start, x_end, y, true);
}

void Line::drawVLine(Image& img, unsigned int y_start, unsigned int y_end, unsigned int x)
{
  drawLine(img, y_start, y_end, x, false);
}

Line::Line(){};

Line::~Line(){};
