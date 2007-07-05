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

void drawLine(Image& image, double x, double y, double x2, double y2,
	      const Image::iterator& color)
{
#if 0
  // asumes RGB for now
  agg::rendering_buffer rbuf (image.getRawData(),
			     image.w, image.h, image.Stride());

  pixfmt pixf (rbuf);
#endif
  
  agg::line_profile_aa profile;
  profile.gamma (agg::gamma_power(1.2));  //optional
  profile.min_width (0.75);               //optional
  profile.smoother_width (3.0);           //optional
  profile.width (1.0);                  //mandatory!
  
  renderer_exact_image ren_base (image);
  renderer_oaa ren (ren_base, profile);

  uint16_t r, g, b;
  color.getRGB (&r, &g, &b);
  ren.color (agg::rgba8(r,g,b));          //mandatory!
  rasterizer_oaa ras (ren);
  
  // ras.round_cap(true);                   //optional
  // ras.accurate_join(true);               //optional
  
  ras.move_to_d (x, y);
  ras.line_to_d (x2, y2);
  ras.render (false);     //false means "don't close"
}

void drawRectange(Image& image, double x, double y, double x2, double y2,
		  const Image::iterator& color)
{
  // top / bottom
  drawLine(image, x,  y,  x2, y,  color);
  drawLine(image, x,  y2, x2, y2, color);

  // sides, avoid dubble set on corners
  drawLine(image, x,  y+1, x,  y2-1, color);
  drawLine(image, x2, y+1, x2, y2-1, color);
}

void drawText(Image& image, double x, double y, char* text, double height,
		const Image::iterator& color)
{
  renderer_exact_image ren_base (image);

  uint16_t r, g, b;
  color.getRGB (&r, &g, &b);
  
  renderer_aa ren_aa(ren_base);
  rasterizer_scanline ras_aa;
  scanline sl;
  
  agg::gsv_text t;
  t.flip (true);
  t.size (height);
  t.text (text);
  t.start_point (x, y);
  agg::conv_stroke<agg::gsv_text> stroke (t);
  stroke.width (1.0);
  ras_aa.add_path (stroke);
  
  ren_aa.color (agg::rgba8(0,0,0));
  agg::render_scanlines (ras_aa, sl, ren_aa);
}
