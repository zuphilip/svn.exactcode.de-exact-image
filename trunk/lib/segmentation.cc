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

// dirty hack to draw a horizontal/vertical red line over the image
void drawLine(Image& img, unsigned int a_start, unsigned int a_end, unsigned int b, bool horizontal)
{
  unsigned int width=(unsigned int) img.w;
  Image::iterator red=img.begin();
  red.setRGB(255, 0, 0);

  unsigned int line=0;
  unsigned int row=0;
  unsigned int max=(horizontal) ? b : a_end;
  Image::iterator i=img.begin();
  Image::iterator end=img.end();
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

void drawHLine(Image& img, unsigned int x_start, unsigned int x_end, unsigned int y)
{
  drawLine(img, x_start, x_end, y, true);
}

void drawVLine(Image& img, unsigned int y_start, unsigned int y_end, unsigned int x)
{
  drawLine(img, y_start, y_end, x, false);
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


bool Segment::Subdivide(Image& img, double tolerance, unsigned int min_length, int fg_threshold, bool horizontal)
{
  unsigned int* counts=Count(img, fg_threshold, horizontal);
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


void Segment::Draw(Image& output)
{
  drawHLine(output, x, x+w-1, y);
  drawHLine(output, x, x+w-1, y+h-1);
  drawVLine(output, y, y+h-1, x);
  drawVLine(output, y, y+h-1, x+w-1);
}


void Segment::InsertChild(unsigned int start, unsigned int end, bool horizontal)
{
  if (horizontal)
    children.push_back(new Segment(x, y+start, w, end-start, this));
  else
    children.push_back(new Segment(x+start, y, end-start, h, this));
}

 
// hack to count foreground pixels in horizontal/vertical lines in gray sub-image
unsigned int* Segment::Count(Image& img, int fg_threshold, bool horizontal)
{
  unsigned int width=(unsigned int) img.w;
  unsigned int* counts=new unsigned int[horizontal ? h : w];
  for (unsigned int n=0; n<(horizontal ? h : w) ; n++)
    counts[n]=0;

  unsigned int line=0;
  unsigned int row=0;

  Image::iterator i=img.begin();
  Image::iterator end=img.end();
  for (; i!=end ; ++i) {

    if (line >= y && line < y+h && row >= x && row < x+w)
      if ((*i).getL() < fg_threshold)
	counts[horizontal ? line-y : row-x]++;
  
    if (++row == width) {
      line++;
      row=0;
    }
  }

  return counts;
}




void segment_recursion(Segment* s, Image& img, double tolerance, unsigned int min_w, unsigned int min_h, int fg_threshold, bool horizontal)
{
  if (s->Subdivide(img, tolerance, horizontal ? min_h : min_w, fg_threshold, horizontal))
    for (unsigned int i=0; i<s->children.size(); i++)
      segment_recursion(s->children[i], img, tolerance, min_w, min_h, fg_threshold, !horizontal);
}

Segment* segment_image(Image& img, double tolerance, unsigned int min_w, unsigned int min_h, int fg_threshold)
{
  Segment* top=new Segment(0, 0, img.w, img.h);
  segment_recursion(top, img, tolerance, min_w, min_h, fg_threshold, true);
  return top;
}
