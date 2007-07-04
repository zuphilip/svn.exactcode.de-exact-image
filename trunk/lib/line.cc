/*
 * Image Line 
 * Copyright (C) 2007 Susanne Klaus, ExactCODE GmbH
 *
 * based on GSMP/plugins-gtk/GUI-gtk/Pixmap.cc:
 * Copyright (C) 2000 - 2002 René Rebe and Valentin Ziegler, GSMP
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

#include "utility/Limits.hh"

template <class T>
class _point {
public:
  typedef T coord;
    
  _point ()
    : x(0), y(0) {
  }
    
  _point (coord i_x, coord i_y)
    : x(i_x), y(i_y) {
  }
    
  coord x, y;
};
  
template <class T>
std::ostream& operator << (std::ostream& strm, const _point<T>& a) {
  strm << "[x: " << a.x << ", y: " << a.y << "]";
  return strm;
}
  
// ***
  
template <class T>
class _area {
public:
    
  typedef T coord;
  typedef _point<coord> point;
    
  _area ()
    : x(0), y(0), w(0), h(0) {
  }
    
  _area (coord i_x, coord i_y, coord i_w, coord i_h)
    : x(i_x), y(i_y), w(i_w), h(i_h) {
  }
    
  void Clip (const _area<T>& a) {
    coord t_x = Utility::limit_min_max (x, a.x, a.x + a.w);
    coord t_y = Utility::limit_min_max (y, a.y, a.y + a.h);
    coord t_x2 = Utility::limit_min_max (x + w, a.x, a.x + a.w);
    coord t_y2 = Utility::limit_min_max (y + h, a.y, a.y + a.h);
      
    x = t_x;
    y = t_y;
    w = t_x2 - t_x;
    h = t_y2 - t_y;
  }
    
  void Extend (const _area<T>& a) {
    coord t_x = std::min (x, a.x);
    coord t_y = std::min (y, a.y);
    coord t_x2 = std::max (x + w, a.x + a.w);
    coord t_y2 = std::max (y + h, a.y + a.h);
      
    x = t_x;
    y = t_y;
    w = t_x2 - t_x;
    h = t_y2 - t_y;
  }
    
  bool Contains (const point& p) const {
    return  (p.x >= x && p.x < x + w &&
	     p.y >= y && p.y < y + h);
  }
    
  bool Overlaps (const _area<T>& a) const {
    if (x < a.x)
      return a.x < x + w && a.y >= y && a.y < y + h;
    else
      return x < a.x + a.w && y >= a.y && y < a.y + a.h;
  }
    
  coord Value () const {
    return w * h;
  }
    
  bool operator== (const _area<T>& a) const {
    return x == a.x && y == a.y && w == a.w && h == a.h;
  }

  bool operator!= (const _area<T>& a) const {
    return ! operator== (a);
  }
    
  coord x, y, w, h;
};

typedef int coord;
typedef _area<coord> area;
typedef area::point point;

inline bool InternalClipLine (point& p1, point& p2, const area& m_area)
{
  // trival reject
  if ( ((p1.y < 0) && (p2.y < 0)) ||
       ((p1.y >= m_area.h) && (p2.y >= m_area.h)) ||
       ((p1.x < 0) && (p2.x < 0)) ||
       ((p1.x >= m_area.w) && (p2.x >= m_area.w)) )
    {
      return false;
    }
  // trival accept or clipping?
  if (!m_area.Contains (p1) || !m_area.Contains (p2) )
    {
      /* clip to top edge */
      if (p1.y < 0)
	{
	  p1.x += (p1.y * (p1.x - p2.x)) / (p2.y - p1.y);
	  p1.y = 0;
	}
      if (p2.y < 0)
	{
	  p2.x += (p2.y * (p1.x - p2.x)) / (p2.y - p1.y);
	  p2.y = 0;
	}
    
      /* clip to bottom edge */
      if (p1.y >= m_area.h)
	{
	  p1.x -= ((m_area.h - p1.y) * (p1.x - p2.x)) / (p2.y - p1.y);
	  p1.y = m_area.h - 1;
	}
      if (p2.y >= m_area.h)
	{
	  p2.x -= ((m_area.h - p2.y) * (p1.x - p2.x)) / (p2.y - p1.y);
	  p2.y = m_area.h - 1;
	}
    
      /* clip to left edge */
      if (p1.x < 0)
	{
	  p1.y += (p1.x * (p1.y - p2.y)) / (p2.x - p1.x);
	  p1.x = 0;
	}
      if (p2.x < 0)
	{
	  p2.y += (p2.x * (p1.y - p2.y)) / (p2.x - p1.x);
	  p2.x = 0;
	}
    
      /* clip to right edge */
      if (p1.x >= m_area.w)
	{
	  p1.y -= ((m_area.w - p1.x) * (p1.y - p2.y)) / (p2.x - p1.x);
	  p1.x = m_area.w - 1;
	}
      if (p2.x >= m_area.w)
	{
	  p2.y -= ((m_area.w - p2.x) * (p1.y - p2.y)) / (p2.x - p1.x);
	  p2.x = m_area.w - 1;
	}
    }
  
  /* make sure p1 is the leftmost */
  if (p1.x > p2.x) {
    point t = p2;
    p2 = p1;
    p1 = t;
  }
  
  return true;
}

inline void Blend (Image::iterator& it, unsigned int x, unsigned int y,
		   const Image::iterator& color, int alpha = 0xff)
{
  it = it.at (x, y);
  
  if (alpha == 0xff) {
    it.set (color);
  }
  else if (alpha > 0) {
    uint16_t r, r2, g, g2, b, b2;
    color.getRGB (&r, &g, &b);
    *it;
    it.getRGB (&r2, &g2, &b2);
    
    const int ialpha = 255 - alpha; // inverse
    
    r = (r * alpha + r2 * ialpha) >> 8;
    g = (g * alpha + g2 * ialpha) >> 8;
    b = (b * alpha + b2 * ialpha) >> 8;
    
    it.setRGB (r, g, b);
    it.set (it);
  }
}

void drawLine (Image& image, unsigned int x, unsigned int y, unsigned int x2, unsigned int y2,
	       const Image::iterator& color)
{
  point p1 (x, y);
  point p2 (x2, y2);
  area m_area (0, 0, image.w, image.h);
  if (!InternalClipLine (p1, p2, m_area))
    return;
  
  coord dx = p2.x - p1.x;
  coord dy = p2.y - p1.y;
  
  Image::iterator it = image.begin();
  
  if (dx == 0) // vertical line
    {
      coord y = std::min (p1.y, p2.y);
      coord t_y = std::max (p1.y, p2.y);
      
      while (y <= t_y){
	Blend (it, p1.x, y, color);
	++y;
      }
    }
  else if (dy == 0) // horizontal line
    {
      coord x = std::min (p1.x, p2.x);
      coord t_x = std::max (p1.x, p2.x);
      
      while (x <= t_x){
	Blend (it, x,  p1.y, color);
	++x;
      }
    }
  // here we draw a line using the midpoint scan-conversion algorithm
  else
    {
      Blend (it, p1.x, p1.y, color);

      if (p2.y > p1.y) { // positive angle
	if (dy <= dx) // flat
	  {
	    coord d = 2 * dy - dx;
	    coord incE = 2 * dy;
	    coord incNE  = 2 * (dy - dx);
	    
	    while (p1.x < p2.x)
	      {
		if (d <= 0) {
		  d += incE;
		}
		else {
		  d += incNE;
		  ++p1.y;
		}
		++p1.x;
		Blend (it, p1.x, p1.y, color);
	      }
	  }
	else // steep (this is a late night guess and might need fixing)
	  {
	    coord d = 2 * dx - dy;
	    coord incE = 2 * dx;
	    coord incNE  = 2 * (dx - dy);
	    
	    while (p1.y < p2.y)
	      {
		if (d <= 0) {
		  d += incE;
		}
		else {
		  d += incNE;
		  ++p1.x;
		}
		++p1.y;
		Blend (it, p1.x, p1.y, color);
	      }
	  }
      }
      else { // negative angle (this is also late night guess and might need fixing)
	if (-dy <= dx) // flat
	  {
	    coord d = 2 * (-dy) - dx;
	    coord incE = 2 * (-dy);
	    coord incNE  = -2 * (dy + dx);
	    
	    while (p1.x < p2.x)
	      {
		if (d <= 0) {
		  d += incE;
		}
		else {
		  d += incNE;
		  -- p1.y;
		}
		++p1.x;
		Blend (it, p1.x, p1.y, color);
	      }
	  }
	else // steep (the last late night guess and might need fixing)
	  {
	    coord d = 2 * (-dx) - dy;
	    coord incE = 2 * dx;
	    coord incNE  = 2 * (dx + dy);
	    
	    while (p1.y > p2.y)
	      {
		if (d <= 0) {
		  d += incE;
		}
		else {
		  d += incNE;
		  ++p1.x;
		}
		--p1.y;
		Blend (it, p1.x, p1.y, color);
	      }
	  }
      }
    }
}

void drawAALine (Image& image, unsigned int x, unsigned int y, unsigned int x2, unsigned int y2,
		 const Image::iterator& color)
{
  point p1 (x, y);
  point p2 (x2, y2);
  area m_area (0, 0, image.w, image.h);
  
  if (!InternalClipLine (p1, p2, m_area))
    return;
  
  coord dx = p2.x - p1.x;
  coord dy = p2.y - p1.y;
  
  Image::iterator it = image.begin();
  
  if (dx == 0 || dy == 0) // vertical or horizontal line
    {
      // reuse
      drawLine (image, x, y, x2, y2, color);
    }
  // here we draw a line using the midpoint scan-conversion algorithm
  else
    {
      if (p2.y > p1.y) { // positive angle
	if (dy <= dx) // flat
	  {
	    coord d = 2 * dy - dx;
	    coord incE = 2 * dy;
	    coord incNE  = 2 * (dy - dx);
	    
	    while (p1.x < p2.x)
	      {
		coord intB = ((d - incNE) << 8) / (incE - incNE + 1);
		coord intA = 255 - intB;
		
		Blend (it, p1.x, p1.y, color, intA);
		if (p1.y+1 < m_area.h)
		  Blend (it, p1.x, p1.y+1, color, intB);
		
		if (d <= 0) {
		  d += incE;
		}
		else {
		  d += incNE;
		  ++p1.y;
		}
		++p1.x;
	      }
	  }
	else // steep (this is a late night guess and might need fixing)
	  {
	    coord d = 2 * dx - dy;
	    coord incE = 2 * dx;
	    coord incNE  = 2 * (dx - dy);
	    
	    while (p1.y < p2.y)
	      {
		coord intB = ((d - incNE) << 8) / (incE - incNE + 1);
		coord intA = 255 - intB;
		
		Blend (it, p1.x, p1.y, color, intA);
		if (p1.x+1 < m_area.w)
		  Blend (it, p1.x+1, p1.y, color, intB);
		
		if (d <= 0) {
		  d += incE;
		}
		else {
		  d += incNE;
		  ++p1.x;
		}
		++p1.y;
	      }
	  }
      }
      else { // negative angle (this is also late night guess and might need fixing)
	if (-dy <= dx) // flat
	  {
	    coord d = 2 * (-dy) - dx;
	    coord incE = 2 * (-dy);
	    coord incNE  = -2 * (dy + dx);
	    
	    while (p1.x < p2.x)
	      {
		coord intB = ((d - incNE) << 8) / (incE - incNE + 1);
		coord intA = 255 - intB;
		
		Blend (it, p1.x, p1.y, color, intA);
		if (p1.y-1 >= 0)
		  Blend (it, p1.x, p1.y-1, color, intB);
		
		if (d <= 0) {
		  d += incE;
		}
		else {
		  d += incNE;
		  --p1.y;
		}
		++ p1.x;
	      }
	  }
	else // steep (the last late night guess and might need fixing)
	  {
	    coord d = 2 * (-dx) - dy;
	    coord incE = 2 * dx;
	    coord incNE  = 2 * (dx + dy);
	    
	    while (p1.y > p2.y)
	      {
		coord intB = ((d - incNE) << 8) / (incE - incNE + 1);
		coord intA = 255 - intB;
		
		// TODO: start pixel is strangely dark
		Blend (it, p1.x, p1.y, color, intA);
		if (p1.x+1 < m_area.w)
		  Blend (it, p1.x+1, p1.y, color, intB);
		
		if (d <= 0) {
		  d += incE;
		}
		else {
		  d += incNE;
		  ++p1.x;
		}
		--p1.y;
	      }
	  }
      }
    }
}

void drawRectange(Image& image, unsigned int x, unsigned int y, unsigned int x2, unsigned int y2,
		  const Image::iterator& color)
{
  // top / bottom
  drawLine(image, x,  y,  x2, y,  color);
  drawLine(image, x,  y2, x2, y2, color);

  // sides, avoid dubble set on corners
  drawLine(image, x,  y+1, x,  y2-1, color);
  drawLine(image, x2, y+1, x2, y2-1, color);
}
