/*
 * Page boundary detection for auto-crop and de-skew.
 * Copyright (C) 2006, 2007 René Rebe
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
#include <vector>

#include "Image.hh"
#include "Colorspace.hh"
#include "crop.hh"
#include "rotate.hh"

//#define DEBUG

#ifdef DEBUG
#include "vectorial.hh"
#endif

#include "deskew.hh"

#include "math/LinearRegression.hh"

#include <functional>

/* We rely on a little hardware support to do reasonble fast but still
   quality auto-crop and de-skew:
   
   The first line(s) that are specified as argument are guaranteed to
   be background raster. So we can simple compare the pixel data to
   know where the background data is replaced by actual scanned data.
   
   This method is so straight forward it also shouldn't be patented
   :-)! -ReneR

   For optimal results at least 4, but even better more, background
   raster lines need to be supplied.
   
   Improvement: Each scanner, especially different model series, has
   it's own noise / dust pattern. To compensate, we use a dynamic
   threshold per column - depending on the average distribution of the
   background raster lines comparing against.
   
   Optimization: As we are only interested in the boundaries of the
   scanned material, we stop searching the inner area after the first
   data points are gathered in all four boundary areas.
   
   Improvement: Border pixels are skipped as they are most often a
   result from clipped paper. Non-neighbor pixels are skipped,
   likewise, as they usually are not part of a boundary line and
   decrease accurancy.
   
   Improvement: We assume the page is centered and only search each
   side / direction half way thru and the side's are are also not
   tracked all the way down thru as the probability of cropping
   increases significantly down to the bottom.
   
   TODO: The deviation could be changed to be more sensitive for
   lighter colors than dark colors, as most scannes produce shadows on
   the borders of the paper that right now decrease accurancy. If we
   want to be fany, we could detect the paper color to decide whether
   to be sensitive or lighter or darker changes.
*/

deskew_rect deskewParameters (Image& image, int raster_rows)
{
#ifdef DEBUG
  const bool debug = true;
#else
  const bool debug = false;
#endif
  
  deskew_rect rect;
  rect.x = 0;
  rect.y = 0;
  rect.width = image.width();
  rect.height = image.height();
  rect.angle = 0;
  
  // dynamic threshold and reference value per column
  Image reference_image; // one pixel high
  reference_image.copyMeta (image);
  reference_image.resize (image.width(), 1);
  std::vector<double> threshold (image.width());
  
  Image::iterator it = image.begin();
  Image::iterator it_ref = reference_image.begin ();
  
  const int border_margin = 2; // skip border pixels, as a cropped edge screws up the line angle
  
  for (int x = 0; x < image.width(); ++x, ++it_ref)
    {
      // average
      double r, g, b, r_avg = 0, g_avg = 0, b_avg = 0;
      for (int y = 0; y < raster_rows; ++y) {
	it = it.at (x, y);
	*it;
	it.getRGB (r, g, b);
	r_avg += r / raster_rows; g_avg += g / raster_rows; b_avg += b / raster_rows;
      }
      
      // deviations -> threshold
      threshold[x] = 0;
      for (int y = 0; y < raster_rows; ++y) {
	it = it.at (x, y);
	*it;
	it.getRGB (r, g, b);
	threshold [x] += (fabs(r_avg-r) + fabs(g_avg-g) + fabs(b_avg-b)) / 3;
      }
      
      // allow a slightly higher threshold than deviation of pixel data
      threshold[x] = 48 * threshold[x] / raster_rows;

      if (threshold[x] < 2./255)
	threshold[x] = 2./255;
      else if (threshold[x] > 32./255)
	threshold[x] = 32./255;
      
      // std::cerr << "[" << x << "]: " << threshold[x] << std::endl;;
      
      it_ref.setRGB (r_avg, g_avg, b_avg);
      it_ref.set(it_ref);
    }
  
  struct comparator {
    bool operator() (Image::iterator& it_ref, Image::iterator& it, double threshold) {
      *it; *it_ref;
      double r, g, b, r2, g2, b2;
      it_ref.getRGB (r, g, b);
      it.getRGB (r2, g2, b2);
      
      // We do not average the RGB values to luminance, as a deviation
      // in one of the channels alone can be significant enough.
      return (fabs(r-r2) + fabs(g-g2) + fabs(b-b2)) > threshold;
    }
  } comparator;

#ifdef DEBUG
  struct marker {
    void operator() (Image::iterator& it, int x, int y, Image::iterator& color) {
      it = it.at (x, y);
      it.set (color);
    }
    void operator() (Image::iterator& it, std::pair<int, int> point, Image::iterator& color) {
      operator () (it, point.first, point.second, color);
    }

    void operator() (Image::iterator& it, int x, int y, Image::iterator& color, double alpha) {
      it = it.at (x, y);
      double r1, r2, g1, g2, b1, b2;
      *it;
      it.getRGB (r1, g1, b1);
      color.getRGB (r2, g2, b2);
      it.setRGB (r1 * (1.-alpha) + r2 * alpha,
                 g1 * (1.-alpha) + g2 * alpha,
                 b1 * (1.-alpha) + b2 * alpha);
      it.set (it);
    }
    void operator() (Image::iterator& it, std::pair<int, int> point, Image::iterator& color, double alpha) {
      operator () (it, point.first, point.second, color, alpha);
    }

  } marker;
#endif
  
  // left and right are x/y flipped due to linear regression
  // calculation - the slope would be near inf. otherwise ...

  std::list<std::pair<int, int> > points_left, points_right, points_top, points_bottom;

  // left
  for (int y = 0; y < image.height() * 5 / 6; ++y)
    {
      it = it.at (0,y);
      it_ref = reference_image.begin ();
      
      for (int x = 0; x < image.width() / 2; ++x, ++it, ++it_ref)
	{
	  if (comparator(it_ref, it, threshold[x]))
	    {
	      if (x > border_margin - 1)
		points_left.push_back (std::pair<int,int> (y, x)); // flipped
	      break;
	    }
	}
    }
  
  // right
  for (int y = 0; y < image.height() * 5 / 6; ++y)
    {
      it = it.at (image.width() - 1, y);
      it_ref = it_ref.at (image.width() - 1, 0);
      
      for (int x = image.width(); x > image.width() / 2; --x, --it, --it_ref)
	{
	  if (comparator(it_ref, it, threshold[x - 1]))
	    {
	      if (x < image.width() - border_margin)
		points_right.push_back (std::pair<int,int> (y, x - 1)); // flipped
	      break;
	    }
	}
    }
  
  // top
  for (int x = 0; x < image.width(); ++x)
    {
      it_ref = it_ref.at (x, 0);
      // this is off-by one intentionally -ReneR
      const int y_offset = raster_rows + 1;
      for (int y = y_offset; y < image.height() / 4; ++y)
	{
	  it = it.at (x, y);
	  if (comparator(it_ref, it, threshold[x]))
	    {
	      if (y > y_offset + border_margin - 1)
		points_top.push_back (std::pair<int,int> (x, y));
	      break;
	    }
	}
    }
  
  // bottom
  for (int x = 0; x < image.width(); ++x)
    {
      it_ref = it_ref.at (x, 0);
      for (int y = image.height(); y > image.height() / 2; --y)
	{
	  it = it.at (x, y - 1);
	  if (comparator(it_ref, it, threshold[x]))
	    {
	      if (y < image.height() - border_margin)
		points_bottom.push_back (std::pair<int,int> (x, y - 1));
	      break;
	    }
	}	  
    }
  
  // just for visualization
  if (debug)
    brightness_contrast_gamma (image, -.75, .0, 1.0);
  
  Image::iterator top_color = image.begin (), bottom_color = image.begin (),
    left_color = image.begin (), right_color = image.begin ();
  
  top_color.setRGB (1., .0, .0); // red
  bottom_color.setRGB (.0, 1., .0); // green
  left_color.setRGB (.0, .0, 1.); // blue
  right_color.setRGB (1., 1., .0); // yellow
  
  struct cleanup {
    void byNeighbor (std::list<std::pair<int, int> >& container, const Image& im,
		     double max_dist = sqrt (2)) {
      std::list<std::pair<int, int> >::iterator it = container.begin ();
      
      while (it != container.end())
	{
	  bool has_neighbor = false;
	  
	  if (it != container.begin()) {
	    std::list<std::pair<int, int> >::iterator it_prev = it;
	    --it_prev;
	    
	    if (dist (it, it_prev) <= max_dist)
	      has_neighbor = true;
	  }
	  
	  if (it != container.end()) {
	    std::list<std::pair<int, int> >::iterator it_next = it;
	    ++it_next;
	    if (it_next != container.end()) {
	      if (dist (it, it_next) <= max_dist)
		has_neighbor = true;
	    }
	  }
	  
	  if (!has_neighbor) {
	    it = container.erase (it);
	    //std::cerr << "removed." << std::endl;
	  }
	  else {
	    //std::cerr << "ok." << std::endl;
	    ++it;
	  }
	}
    }
    
    void byDistance (std::list<std::pair<int, int> >& container, LinearRegression<double>& lr,
		     double max_dist = 32) {
      std::list<std::pair<int, int> >::iterator it = container.begin ();
      
      while (it != container.end())
	{
	  const double estimated_y = lr.estimateY (it->first);
	  const double d = std::abs (estimated_y - it->second);
	  
	  //std::cerr << "[" << it->first << "," << it->second
	  //          << "] to estimate [" << it->first << "," << estimated_y
	  //          << "] dist: " << d << std::endl;
	  
	  if (d > max_dist) {
	    it = container.erase (it);
	    //std::cerr << "removed." << std::endl;
	  } 
	  else {
	    //std::cerr << "ok." << std::endl;
	    ++it;
	  }
	}
    }
    
  private:
    double dist (std::list<std::pair<int, int> >::iterator it1,
		 std::list<std::pair<int, int> >::iterator it2)
    {
      const double xdist = std::abs (it1->first - it2->first);
      const double ydist = std::abs (it1->second - it2->second);
      
      const double d = sqrt (xdist * xdist + ydist * ydist);

      //std::cerr << "[" << it1->first << "," << it1->second
      //	<< "] to [" << it2->first << "," << it2->second
      //	<< "] dist: " << d << std::endl;
      
      return d;
    }
  } cleanup;

  cleanup.byNeighbor (points_top, image);
  cleanup.byNeighbor (points_bottom, image);
  cleanup.byNeighbor (points_left, image);
  cleanup.byNeighbor (points_right, image);
  
  LinearRegression<double> reg_top, reg_bottom, reg_left, reg_right;
  
  reg_top.addRange (points_top.begin(), points_top.end());
  reg_bottom.addRange (points_bottom.begin(), points_bottom.end());
  reg_left.addRange (points_left.begin(), points_left.end());
  reg_right.addRange (points_right.begin(), points_right.end());
  
  class Line {
  public:
    typedef std::pair <double, double> point;
    
    Line (point _p1, point _p2)
      : p1 (_p1), p2 (_p2) {};
    
    Line (double x1, double y1, double x2, double y2)
      : p1 (x1, y1), p2 (x2, y2) {};
    
    bool intersection (const Line& other, point& p)
    {
      const double det =
	(other.p2.second - other.p1.second) * (p2.first - p1.first) -
	(other.p2.first - other.p1.first) * (p2.second - p1.second);
      
      if (det == 0)  // parallel
	return false;
      
      double Ua =
	(other.p2.first - other.p1.first) * (p1.second - other.p1.second) -
	(other.p2.second - other.p1.second) * (p1.first - other.p1.first);
      
      Ua /= det;
      
      p.first = p1.first + Ua * (p2.first - p1.first);
      p.second = p1.second + Ua * (p2.second - p1.second);
      return true;
    }
    
    point& begin () { return p1; };
    point& end () { return p2; };
    
    point mid () {
      return point (p1.first + (p2.first - p1.first) / 2,
		    p1.second + (p2.second - p1.second) / 2);
    };
    
    double length () {
      const double xrel = std::abs (p1.first - p2.first);
      const double yrel = std::abs (p1.second - p2.second);
      return sqrt (xrel*xrel + yrel*yrel);
    }
    
#ifdef DEBUG
    // just quick
    void draw (Path& path, Image& image, const Image::iterator& color) {
      double r, g, b; color.getRGB (r, g, b);
      path.setFillColor (r, g, b);
      path.moveTo (p1.first, p1.second);
      path.addLineTo (p2.first, p2.second);
      path.draw (image);
      path.clear ();
    };
#endif
    
  private:
    point p1, p2;
  };

#ifdef DEBUG  
  Path path;
  // just for visualization, draw markers
  {
    for (std::list<std::pair<int,int> >::iterator p = points_top.begin();
         p != points_top.end(); ++p)
      marker (it, p->first, p->second, top_color, .5);

    for (std::list<std::pair<int,int> >::iterator p = points_bottom.begin();
         p != points_bottom.end(); ++p)
      marker (it, p->first, p->second, bottom_color, .5);

    for (std::list<std::pair<int,int> >::iterator p = points_left.begin();
         p != points_left.end(); ++p)
      marker (it, p->second, p->first, left_color, .5); // flipped

    for (std::list<std::pair<int,int> >::iterator p = points_right.begin();
         p != points_right.end(); ++p)
      marker (it, p->second, p->first, right_color, .5); // flipped


    path.setLineWidth (0.75);
    double dashes [] = { 12, 6 };
    path.setLineDash (0, dashes, 2);
    
    Line line_top_pre (0, reg_top.getA (), image.width(), reg_top.estimateY (image.width()));
    Line line_bottom_pre (0, reg_bottom.getA (), image.width(), reg_bottom.estimateY (image.width()));
    Line line_left_pre (reg_left.getA (), 0, reg_left.estimateY (image.height()), image.height());
    Line line_right_pre (reg_right.getA (), 0, reg_right.estimateY (image.height()), image.height());
    
    line_top_pre.draw (path, image, top_color);
    line_bottom_pre.draw (path, image, bottom_color);
    line_left_pre.draw (path, image, left_color);
    line_right_pre.draw (path, image, right_color);
    
    path.setLineDash (0, NULL, 0);
    path.setLineWidth (1.0);
  }
#endif
  
  // clean after first linear regression, flatten extrema

  cleanup.byDistance (points_top, reg_top);
  cleanup.byDistance (points_bottom, reg_bottom);
  cleanup.byDistance (points_left, reg_left);
  cleanup.byDistance (points_right, reg_right);
  
  // re-calculate
  
  reg_top.clear (); reg_bottom.clear (); reg_left.clear (); reg_right.clear ();
    
  reg_top.addRange (points_top.begin(), points_top.end());
  reg_bottom.addRange (points_bottom.begin(), points_bottom.end());
  reg_left.addRange (points_left.begin(), points_left.end());
  reg_right.addRange (points_right.begin(), points_right.end());

#ifdef DEBUG
  // just for visualization, draw markers
  {
    for (std::list<std::pair<int,int> >::iterator p = points_top.begin();
	 p != points_top.end(); ++p)
      marker (it, p->first, p->second, top_color);
  
    for (std::list<std::pair<int,int> >::iterator p = points_bottom.begin();
	 p != points_bottom.end(); ++p)
      marker (it, p->first, p->second, bottom_color);
  
    for (std::list<std::pair<int,int> >::iterator p = points_left.begin();
	 p != points_left.end(); ++p)
      marker (it, p->second, p->first, left_color); // flipped
  
    for (std::list<std::pair<int,int> >::iterator p = points_right.begin();
	 p != points_right.end(); ++p)
      marker (it, p->second, p->first, right_color); // flipped

    std::cerr << "top: " << reg_top << std::endl
	      << "bottom: " << reg_bottom << std::endl
	      << "left: " << reg_left << std::endl
	      << "right: " << reg_right << std::endl;
  }
#endif

  Line line_left (reg_left.getA (), 0, reg_left.estimateY (image.height()), image.height());
  Line line_right (reg_right.getA (), 0, reg_right.estimateY (image.height()), image.height());
  Line line_top (0, reg_top.getA (), image.width(), reg_top.estimateY (image.width()));
  Line line_bottom (0, reg_bottom.getA (), image.width(), reg_bottom.estimateY (image.width()));
  
  // if there is no bottom data, default to the last scanline
  // TODO: simillar safeguards should probably added for the other borders
  if (fabs(line_bottom.begin().second) < 1.0)
    line_bottom = Line (0, image.height() - 1, image.width(), image.height() - 1);

#ifdef DEBUG
  {
    line_top.draw (path, image, top_color);
    line_left.draw (path, image, left_color);
    line_right.draw (path, image, right_color);
    line_bottom.draw (path, image, bottom_color);
  }
#endif

  Line::point p1, p2, p3, p4;
  if (line_top.intersection (line_left, p1) &&
      line_top.intersection (line_right, p2) &&
      line_bottom.intersection (line_left, p3) &&
      line_bottom.intersection (line_right, p4))
    {
      line_top = Line (p1, p2);
      line_bottom = Line (p3, p4);
      line_left = Line (p1, p3);
      line_right = Line (p2, p4);
      
      Line line_width = Line (line_left.mid(), line_right.mid());
      Line line_height = Line (line_top.mid(), line_bottom.mid());

      // average angle
      double angle = (reg_top.getB() +
		      reg_bottom.getB() +
		      - reg_left.getB() +
		      - reg_left.getB() ) / 4;
      
      angle = atan (angle) / M_PI * 180;
      
      rect.x = p1.first;
      rect.y = p1.second;
      rect.width = line_width.length();
      rect.height = line_height.length();
      rect.angle = -angle;
      
#ifdef DEBUG
	std::cerr << "angle: " << angle << std::endl;
	
	Image::iterator note_color = image.begin();
	note_color.setRGB (1.0, 1.0, 1.0);
	line_width.draw (path, image, note_color);
	line_height.draw (path, image, note_color);
#endif
    }
  else
    std::cerr << "lines parallel!?" << std::endl;
  
  return rect;
}

bool deskew (Image& image, const int raster_rows)
{
  deskew_rect rect = deskewParameters (image, raster_rows);
  
#ifndef DEBUG
  // TODO: fill with nearest in-document color, or so ...
  Image::iterator background = image.begin(); background.setL (255);
  
  if (fabs(rect.angle) > 0.01) {
    Image* cropped_image =
      copy_crop_rotate (image,
			(unsigned int) rect.x, (unsigned int) rect.y,
			(unsigned int) rect.width, (unsigned int) rect.height,
			rect.angle, background);
    image.copyTransferOwnership (*cropped_image);
    image.copyMeta (*cropped_image);
  } else {
    crop (image,
	  (unsigned int) rect.x, (unsigned int) rect.y,
	  (unsigned int) rect.width, (unsigned int) rect.height);
  }
#endif
}
