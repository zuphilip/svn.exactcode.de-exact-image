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


#include <vector>
#include "Image.hh"

class Segment
{
public:
  unsigned int x;
  unsigned int y;
  unsigned int w;
  unsigned int h;
  Segment* parent;
  std::vector <Segment*> children;

  Segment(unsigned int ix, unsigned int iy, unsigned int iw, unsigned int ih, Segment* iparent=0);
  ~Segment();

  bool Subdivide(Image& img, double tolerance, unsigned int min_length, int fg_threshold, bool horizontal);

  // Draws a red frame around the segment
  void Draw(Image& output);

private:

  void InsertChild(unsigned int start, unsigned int end, bool horizontal);

  // hack to count foreground pixels in horizontal/vertical lines in gray sub-image
  unsigned int* Count(Image& img, int fg_threshold, bool horizontal);
};



// returns a segmentation of <img>. preprocessing using optimize2bw is recommended.
// <tolerance> is the maximum fraction of foreground pixels allowed in a separator line
// <fg_threshold> is the maximum luminance of a foreground pixel
// <min_w> and <min_h> denote the minimum separator width and height
Segment* segment_image(Image& img, double tolerance, unsigned int min_w, unsigned int min_h, int fg_threshold);

