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


#include "segmentation.hh"

// dirty hack to draw a horizontal/vertical line over the image
void drawLine(Image& img, unsigned int a_start, unsigned int a_end, unsigned int b, bool horizontal,
	      unsigned int R=255, unsigned int G=0, unsigned int B=0)
{
  unsigned int width=(unsigned int) img.w;
  Image::iterator color=img.begin();
  color.setRGB(R, G, B);
  
  unsigned int line=0;
  unsigned int row=0;
  unsigned int max=(horizontal) ? b : a_end;
  Image::iterator i=img.begin();
  Image::iterator end=img.end();
  for (; i!=end && line <= max; ++i) {
    if (horizontal) {
      if (line==b && row >= a_start && row <= a_end)
	i.set(color);
    } else {
      if (row==b && line >= a_start && line <= a_end)
	i.set(color);
    }
    if (++row == width) {
      line++;
      row=0;
    }
  }
}

void drawHLine(Image& img, unsigned int x_start, unsigned int x_end, unsigned int y,
	       unsigned int r=255, unsigned int g=0, unsigned int b=0)
{
  drawLine(img, x_start, x_end, y, true, r,g,b);
}

void drawVLine(Image& img, unsigned int y_start, unsigned int y_end, unsigned int x,
	       unsigned int r=255, unsigned int g=0, unsigned int b=0)
{
  drawLine(img, y_start, y_end, x, false, r,g,b);
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
