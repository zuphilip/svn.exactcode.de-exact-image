/*
 * Image Segmentation
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
#include "segmentation.hh"

void drawHLine(Image& img, unsigned int x_start, unsigned int x_end, unsigned int y,
	       unsigned int r=255, unsigned int g=0, unsigned int b=0)
{
  Image::iterator color = img.begin();
  color.setRGB (r, g, b);
  
  drawLine(img, x_start, y, x_end, y, color);
}

void drawVLine(Image& img, unsigned int y_start, unsigned int y_end, unsigned int x,
	       unsigned int r=255, unsigned int g=0, unsigned int b=0)
{

  Image::iterator color = img.begin();
  color.setRGB (r, g, b);
  
  drawLine(img, x, y_start, x, y_end, color);
}





Segment::Segment(unsigned int ix, unsigned int iy, unsigned int iw, unsigned int ih, Segment* iparent)
{
  x=ix;
  y=iy;
  w=iw;
  h=ih;
  parent=iparent;
}


Segment::~Segment()
{
  for (unsigned int i=0; i<children.size(); i++)
    delete children[i];
}


bool Segment::Subdivide(const FGMatrix& img, double tolerance, unsigned int min_length, bool horizontal)
{
  unsigned int* counts=Count(img, horizontal);
  unsigned int end=horizontal ? h : w;
  unsigned int max_pixels=(unsigned int) (tolerance*(double)(horizontal ? w : h));

  unsigned int length=0;
  unsigned int last_start=0;
  for (unsigned int n=0; n<end; n++) {
    if (counts[n] <= max_pixels) {
      length++;
    }
    else {
      if (length >= min_length || length==n) {
	if (length < n)
	  InsertChild(last_start, n-length, horizontal);
	last_start=n;
      }
      length=0;
    }

  }
  if (last_start > 0)
    InsertChild(last_start, end-length, horizontal);
  
  delete[] counts;
  return (children.size() > 0);
}


void Segment::Draw(Image& output, unsigned int r, unsigned int g, unsigned int b)
{
  drawHLine(output, x, x+w-1, y, r,g,b);
  drawHLine(output, x, x+w-1, y+h-1, r,g,b);
  drawVLine(output, y, y+h-1, x, r,g,b);
  drawVLine(output, y, y+h-1, x+w-1, r,g,b);
}


void Segment::InsertChild(unsigned int start, unsigned int end, bool horizontal)
{
  if (horizontal)
    children.push_back(new Segment(x, y+start, w, end-start, this));
  else
    children.push_back(new Segment(x+start, y, end-start, h, this));
}

 
// count foreground pixels in horizontal/vertical lines
unsigned int* Segment::Count(const FGMatrix& img, bool horizontal)
{
  FGMatrix subimg(img, x, y, w, h);
  unsigned int* counts=new unsigned int[horizontal ? h : w];
  for (unsigned int n=0; n<(horizontal ? h : w) ; n++)
    counts[n]=0;

  for (unsigned int px=0; px<w; px++)
    for (unsigned int py=0; py<h; py++)
      if (subimg(px,py))
	counts[horizontal ? py : px]++;

  return counts;
}




void segment_recursion(Segment* s, const FGMatrix& img, double tolerance, unsigned int min_w, unsigned int min_h, bool horizontal)
{
  if (s->Subdivide(img, tolerance, horizontal ? min_h : min_w, horizontal))
    for (unsigned int i=0; i<s->children.size(); i++)
      segment_recursion(s->children[i], img, tolerance, min_w, min_h, !horizontal);
}

Segment* segment_image(const FGMatrix& img, double tolerance, unsigned int min_w, unsigned int min_h)
{
  Segment* top=new Segment(0, 0, img.w, img.h);
  segment_recursion(top, img, tolerance, min_w, min_h, true);
  return top;
}
