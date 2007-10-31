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

#include "vectorial.hh"
#include <iostream>

#include "agg.hh"
#include "Image.hh"

#include "agg_conv_dash.h"
#include "agg_conv_curve.h"
#include "agg_conv_smooth_poly1.h"
#include "agg_path_storage.h"

// ---

Path::Path ()
  : line_width (1.0), dashes_start_offset (0.0),
    line_cap (agg::butt_cap), line_join (agg::miter_join)
{
}

Path::~Path ()
{
}

void Path::moveTo (double x, double y)
{
  path.move_to (x, y);
}

void Path::addLineTo (double x, double y)
{
  path.line_to (x, y);
}

void Path::addArcTo (double rx, double ry,  double angle,
		     double x, double y)
{
  path.arc_to (rx, ry, angle,
	       false /*large arc */, false /* sweep */,
	       x, y);
}

void Path::addArc (double rx, double ry,  double angle,
		     double dx, double dy)
{
  path.arc_to (rx, ry, angle,
	       false /*large arc */, false /* sweep */,
	       dx, dy);
}

void Path::addCurveTo (double c1x, double c1y,
		       double x, double y)
{
  path.curve3 (c1x, c1y, x, y);
}

void Path::addCurveTo (double c1x, double c1y, double c2x, double c2y,
		       double x, double y)
{
  path.curve4 (c1x, c1y, c2x, c2y, x, y);
}

void Path::end ()
{
  // TODO: check if we need to pass an arg to not implicitly close the path
  path.end_poly ();
}

void Path::close ()
{
  path.close_polygon ();
}

void Path::setFillColor (double _r, double _g, double _b, double _a)
{
  r = _r;
  g = _g;
  b = _b;
  a = _a;
}

void Path::setLineWidth (double width)
{
  line_width = width;
}

void Path::setLineDash (double offset, const std::vector<double>& _dashes)
{
  dashes_start_offset = offset;
  dashes = _dashes;
}

void Path::setLineDash (double offset, const double* _dashes, int n)
{
  dashes_start_offset = offset;
  dashes.clear ();
  for (; n; n--, _dashes++)
    dashes.push_back (*_dashes);
}

void Path::setLineCap (line_cap_t cap)
{
  line_cap = cap;
}

void Path::setLineJoin (line_join_t join)
{
  line_join = join;
}

void Path::draw (Image& image, filling_rule_t fill)
{
  renderer_exact_image ren_base (image);
  
  renderer_aa ren (ren_base);
  ren.color (agg::rgba8 (255*r, 255*g, 255*b, 255*a));
  
  rasterizer_scanline ras;
  scanline sl;

  agg::conv_curve<agg::path_storage> smooth (path);
  
  if (fill == fill_none)
    {
      agg::line_profile_aa profile;
      profile.gamma (agg::gamma_power(1.2)); // optional
      //profile.min_width (0.75); // optional
      //profile.smoother_width (3.0); //optional
      
      if (dashes.empty ())
	{
	  agg::conv_stroke<agg::conv_curve<agg::path_storage> > stroke (smooth);
	  
	  stroke.line_cap (line_cap);
	  stroke.line_join (line_join);
	  stroke.width (line_width);
	  
	  ras.add_path (stroke);
	}
      else
	{
	  typedef agg::conv_dash<agg::conv_curve<agg::path_storage> > dash_t;
	  dash_t dash (smooth);
	  dash.dash_start (dashes_start_offset);
	  for (std::vector<double>::const_iterator i = dashes.begin ();
	       i != dashes.end ();) {
	    double a = *i++, b;
	    if (i != dashes.end ())
	      b = *i++;
	    else
	      break; // TODO: warn or exception ?
	    dash.add_dash (a, b);
	  }
	  
	  agg::conv_stroke<dash_t> stroke (dash);
	  
	  stroke.line_cap (line_cap);
	  stroke.line_join (line_join);
	  stroke.width (line_width);
	  
	  ras.add_path (stroke);
	}
    }
  else {
    // just cast, we use a toll-free enum bridge
    ras.filling_rule ((agg::filling_rule_e)fill);
    ras.add_path (smooth);
  }
  
  agg::render_scanlines (ras, sl, ren);
}

// --

void drawLine(Image& image, double x, double y, double x2, double y2,
	      const Image::iterator& color, const drawStyle& style)
{
  Path path;
  path.moveTo (x, y);
  path.addLineTo (x2, y2);

  path.setLineWidth (style.width);
  path.setLineDash (0, style.dash);
  double r, g, b;
  color.getRGB (r, g, b);
  path.setFillColor (r, g, b);
  
  path.draw (image);
}

void drawRectangle(Image& image, double x, double y, double x2, double y2,
		   const Image::iterator& color, const drawStyle& style)
{
  Path path;
  path.moveTo (x, y);
  path.addLineTo (x2, y);
  path.addLineTo (x2, y2);
  path.addLineTo (x, y2);
  path.close ();
  
  path.setLineWidth (style.width);
  path.setLineDash (0, style.dash);
  path.setLineJoin (agg::miter_join);
  
  double r, g, b;
  color.getRGB (r, g, b);
  path.setFillColor (r, g, b);
  
  path.draw (image);
}

void drawText (Image& image, double x, double y, char* text, double height,
	       const Image::iterator& color)
{
  renderer_exact_image ren_base (image);

  uint16_t r, g, b;
  color.getRGB (&r, &g, &b);
  
  renderer_aa ren (ren_base);
  rasterizer_scanline ras;
  scanline sl;
  
  agg::gsv_text t;
  t.flip (true);
  t.size (height);
  t.text (text);
  t.start_point (x, y);
  agg::conv_stroke<agg::gsv_text> stroke (t);
  stroke.width (1.0);
  ras.add_path (stroke);
  
  ren.color (agg::rgba8 (r, g, b));
  agg::render_scanlines (ras, sl, ren);
}
