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
#include "agg_path_storage.h"

void renderPath (Image& image, agg::path_storage& path,
		 const Image::iterator& color, const drawStyle& style)
{
  renderer_exact_image ren_base (image);
  
  uint16_t r, g, b;
  color.getRGB (&r, &g, &b);

  agg::line_profile_aa profile;
  profile.gamma (agg::gamma_power(1.2)); // optional
  profile.min_width (0.75); // optional
  profile.smoother_width (3.0); //optional
  profile.width (style.width); // mandatory!
  
  renderer_aa ren (ren_base);
  ren.color (agg::rgba8 (r, g, b));

  rasterizer_scanline ras;
  scanline sl;
  
  if (style.dash.empty ())
    {
      agg::conv_stroke<agg::path_storage> stroke (path);
      
      //stroke.line_cap (cap);
      stroke.line_cap (agg::round_cap);
      stroke.line_join (agg::round_join);
      stroke.width (style.width);
      
      ras.add_path (stroke);
    }
  else
    {
      typedef agg::conv_dash<agg::path_storage> dash_t;
      dash_t dash (path);
      //dash.dash_start (offset);
      for (std::vector<double>::const_iterator i = style.dash.begin ();
	   i != style.dash.end ();) {
	double a = *i++, b;
	if (i != style.dash.end ())
	  b = *i++;
	else
	  break; // TODO: warn or exception ?
	dash.add_dash (a, b);
      }
      
      agg::conv_stroke<dash_t> stroke (dash);
      
      //stroke.line_cap (cap);
      //stroke.line_join (join);
      stroke.width (style.width);
	
      ras.add_path (stroke);
    }

  agg::render_scanlines (ras, sl, ren);
}

void drawLine(Image& image, double x, double y, double x2, double y2,
	      const Image::iterator& color, const drawStyle& style)
{
  agg::path_storage path;
  
  path.move_to (x, y);
  path.line_to (x2, y2);
  
  renderPath (image, path, color, style);
}

void drawRectangle(Image& image, double x, double y, double x2, double y2,
		   const Image::iterator& color, const drawStyle& style)
{
  agg::path_storage path;
  
  path.move_to (x, y);
  path.line_to (x2, y);
  path.line_to (x2, y2);
  path.line_to (x, y2);
  path.close_polygon ();
  
  renderPath (image, path, color, style);
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
