/*
 * Copyright (c) 2008 Rene Rebe <rene@exactcode.de>
 *
 * Based on the svg_test application window,
 * Copyright (C) 2002-2005 Maxim Shemanarev (http://www.antigrain.com)
 */

#include "agg_basics.h"
#include "agg_rendering_buffer.h"
#include "agg_rasterizer_scanline_aa.h"
#include "agg_scanline_p.h"
#include "agg_renderer_scanline.h"
#include "agg_pixfmt_rgba.h"

#include "agg_svg_parser.hh"

#include "svg.hh"

#include "agg.hh" // EI Agg

bool SVGCodec::readImage (std::istream* stream, Image& image)
{
  agg::svg::path_renderer m_path;
  agg::svg::parser p (m_path);
  
  try
    {
      p.parse(*stream);
    }
  catch(agg::svg::exception& e)
    {
      std::cerr << e.msg() << std::endl;
      return false;
    }
  
  double m_min_x = 0;
  double m_min_y = 0;
  double m_max_x = 0;
  double m_max_y = 0;
  
  const double m_expand = 0;
  const double m_gamma = 1;
  const double m_scale = 1;
  const double m_rotate = 0;
  
  m_path.arrange_orientations ();
  m_path.bounding_rect (&m_min_x, &m_min_y, &m_max_x, &m_max_y);
  
  image.bps = 8; image.spp = 3;
  image.resize (m_max_x - m_min_x, m_max_y - m_min_y);
  
  renderer_exact_image rb (image);
  typedef agg::renderer_scanline_aa_solid<renderer_base> renderer_solid;
  renderer_solid ren (rb);
  
  rb.clear (agg::rgba(1,1,1));
  
  agg::rasterizer_scanline_aa<> ras;
  agg::scanline_p8 sl;
  agg::trans_affine mtx;
  
  ras.gamma(agg::gamma_power(m_gamma));
  mtx *= agg::trans_affine_translation ((m_min_x + m_max_x) * -0.5,
					(m_min_y + m_max_y) * -0.5);
  mtx *= agg::trans_affine_scaling (m_scale);
  mtx *= agg::trans_affine_rotation (agg::deg2rad(m_rotate));
  mtx *= agg::trans_affine_translation ((m_min_x + m_max_x) * 0.5,
					(m_min_y + m_max_y) * 0.5);
  
  m_path.expand(m_expand);
  //start_timer();
  m_path.render(ras, sl, ren, mtx, rb.clip_box(), 1.0);
  //double tm = elapsed_time();
  unsigned vertex_count = m_path.vertex_count();
  
  //std::cerr << "Vertices=" << vertex_count << " Time=" << tm " ms" std::endl;
  
  return true;
}

bool SVGCodec::writeImage (std::ostream* stream, Image& image, int quality,
                           const std::string& compress)
{
  return false;
}

SVGCodec svg_loader;
