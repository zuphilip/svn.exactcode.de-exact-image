/*
 * Agg vector rasterization interface
 * Copyright (C) 2007 - 2008 Rene Rebe, ExactCODE GmbH
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
 * Alternatively, commercial licensing options are available from the
 * copyright holder ExactCODE GmbH Germany.
 */

#include "vectorial.hh"
#include <iostream>

#include "agg.hh"
#include "Image.hh"

#include "agg_conv_dash.h"
#include "agg_conv_curve.h"
#include "agg_conv_contour.h"
#include "agg_conv_segmentator.h"
#include "agg_conv_smooth_poly1.h"
#include "agg_path_storage.h"
#include "agg_trans_single_path.h"

#if WITHFREETYPE == 1
#include "agg_font_freetype.h"
#endif

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

void Path::addRect (double x, double y, double x2, double y2)
{
  moveTo (x, y);
  addLineTo (x2, y);
  addLineTo (x2, y2);
  addLineTo (x, y2);
  close ();
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
  path.arc_rel (rx, ry, angle,
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
/*
void Path::addText (char* text, double height)
{
  agg::gsv_text t;
  t.flip (true);
  t.size (height);
  t.text (text);
  t.start_point (x, y);
  
  agg::conv_stroke<agg::gsv_text> stroke (t);
  stroke.width (1.0);
  ras.add_path (stroke);
  
  ren.color (agg::rgba8(r, g, b, a));
  agg::render_scanlines (ras, sl, ren);
}
*/
void Path::end ()
{
  // do not close the path, we have ::close() for that purpose
  path.end_poly (agg::path_flags_none);
}

void Path::close ()
{
  path.close_polygon ();
}

void Path::clear ()
{
  path.remove_all ();
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
  
  /* the rederer_bin type would "avoid" anti-aliasing */
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

#if WITHFREETYPE == 1

static const bool hinting = true;
static const bool kerning = true;

static const char* font = "/usr/X11/share/fonts/TTF/DejaVuSansBold.ttf"
//"/usr/X11/share/fonts/TTF/DejaVuSans.ttf"
//"/home/rene/.fonts/pala.ttf"
;

// Attention! Right now the horizontal and on path
// produce different strokes if non-zero. Review and
// fix if changed or exported to the outside.
static const double weight = 0;

void Path::drawText (Image& image, const char* text, double height)
{
  renderer_exact_image ren_base (image);
  
  rasterizer_scanline ras;
  scanline sl;
  
  renderer_aa ren_solid (ren_base);
  ren_solid.color (agg::rgba8 (255*r, 255*g, 255*b, 255*a));

  renderer_bin ren_bin (ren_base);
  ren_bin.color (agg::rgba8 (255*r, 255*g, 255*b, 255*a));
  
  typedef agg::font_engine_freetype_int32 font_engine_type;
  typedef agg::font_cache_manager<font_engine_type> font_manager_type;
  
  font_engine_type m_feng;
  font_manager_type m_fman (m_feng);
  
  // Pipeline to process the vectors glyph paths (curves + contour)
  agg::conv_curve<font_manager_type::path_adaptor_type> m_curves (m_fman.path_adaptor());
  agg::conv_contour<agg::conv_curve<font_manager_type::path_adaptor_type> > m_contour (m_curves);
  agg::glyph_rendering gren = agg::glyph_ren_outline;
  
  if (!m_feng.load_font (font, 0, gren))
    {
      std::cerr << "failed to load ttf font" << std::endl;
      return;
    }
  
  m_feng.hinting (hinting);
  m_feng.height (height);
  m_feng.width (height);
  m_feng.flip_y (true);
  
  m_contour.width (-weight * height * 0.05);

  const char* p = text;
  
  double x = path.last_x(), y = path.last_y();  
  while (*p)
    {
      const agg::glyph_cache* glyph = m_fman.glyph (*p);
      if (glyph)
	{
	  if (kerning) {
	    m_fman.add_kerning(&x, &y);
	  }
	  
	  m_fman.init_embedded_adaptors (glyph, x, y);

	  switch (glyph->data_type)
	    {
	    case agg::glyph_data_mono:
	      agg::render_scanlines (m_fman.mono_adaptor(), 
				     m_fman.mono_scanline(), 
				     ren_bin);
	      break;

	    case agg::glyph_data_gray8:
	      ren_solid.color (agg::rgba8 (255*r, 255*g, 255*b, 255*a));
	      agg::render_scanlines (m_fman.gray8_adaptor(), 
				     m_fman.gray8_scanline(), 
				     ren_solid);
	      break;

	    case agg::glyph_data_outline:
	      // Skip the contour converter for weight about zero.
	      if (fabs (weight <= 0.01))
		ras.add_path (m_curves);
	      else
		ras.add_path (m_contour);
	      break;
	      
	    default: break;
	    }

	  // increment pen position
	  x += glyph->advance_x;
	  y += glyph->advance_y;
	}
      ++p;
    }
  
  agg::render_scanlines (ras, sl, ren_solid);
  
  path.move_to (x, y); // save last point for further drawing
}

void Path::drawTextOnPath (Image& image, const char* text, double height)
{
  renderer_exact_image ren_base (image);
  
  renderer_aa ren_solid (ren_base);
  ren_solid.color (agg::rgba8 (255*r, 255*g, 255*b, 255*a));

  rasterizer_scanline ras;
  scanline sl;
  
  agg::conv_curve<agg::path_storage> smooth (path);
  agg::trans_single_path tcurve;
  tcurve.add_path (smooth);
  // tcurve.preserve_x_scale(m_preserve_x_scale.status());
  
  typedef agg::font_engine_freetype_int32 font_engine_type;
  typedef agg::font_cache_manager<font_engine_type> font_manager_type;
  
  font_engine_type m_feng;
  font_manager_type m_fman (m_feng);
  
  // Pipeline to process the vectors glyph paths (curves + contour)
  agg::conv_curve<font_manager_type::path_adaptor_type> m_curves (m_fman.path_adaptor());
  agg::conv_contour<agg::conv_curve<font_manager_type::path_adaptor_type> > m_contour (m_curves);
  agg::glyph_rendering gren = agg::glyph_ren_outline;
  
  if (!m_feng.load_font (font, 0, gren))
    {
      std::cerr << "failed to load ttf font" << std::endl;
      return;
    }
  
  // Transform pipeline
  typedef agg::conv_curve<font_manager_type::path_adaptor_type> conv_font_curve_type;
  typedef agg::conv_segmentator<conv_font_curve_type> conv_font_segm_type;
  typedef agg::conv_transform<conv_font_segm_type, agg::trans_single_path> conv_font_trans_type;
  conv_font_curve_type fcurves (m_fman.path_adaptor());
  
  conv_font_segm_type  fsegm (fcurves);
  conv_font_trans_type ftrans (fsegm, tcurve);
  fsegm.approximation_scale (3.0);
  fcurves.approximation_scale (2.0);
  
  m_feng.hinting (hinting);
  m_feng.height (height);
  m_feng.width (height);
  m_feng.flip_y (true);
  
  m_contour.width (-weight * height * 0.05);
  
  const char* p = text;
  
  ras.reset ();
  
  double x = 0, y = 3;  
  while (*p)
    {
      const agg::glyph_cache* glyph = m_fman.glyph (*p);
      if (glyph)
	{
	  if (kerning) {
	    m_fman.add_kerning(&x, &y);
	  }
	  
	  m_fman.init_embedded_adaptors (glyph, x, y);
	  
	  if (glyph->data_type == agg::glyph_data_outline)
	    {
	      ras.add_path (ftrans);
	    }
	  else
	    std::cerr << "Warning: unexpected glyph type!" << std::endl;
	  
	  // increment pen position
	  x += glyph->advance_x;
	  y += glyph->advance_y;
	}
      ++p;
    }
  
    agg::render_scanlines(ras, sl, ren_solid);
    
    path.move_to (x, y); // save last point for further drawing
}

#endif