/*
 * Copyright (C) 2006 - 2015 René Rebe, ExactCODE GmbH Germany.
 *           (C) 2006, 2007 Archivista GmbH, CH-8042 Zuerich
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

#ifndef SCALE_HH
#define SCALE_HH

// pick the best
void scale (Image& image, double xscale, double yscale, bool fixed = false);

// explicit versions
void nearest_scale (Image& image, double xscale, double yscale, bool fixed = false);
void box_scale (Image& image, double xscale, double yscale, bool fixed = false);

void bilinear_scale (Image& image, double xscale, double yscale, bool fixed = false);
void bicubic_scale (Image& image, double xscale, double yscale, bool fixed = false);

void ddt_scale (Image& image, double xscale, double yscale, bool fixed = false, bool extended = true);

void thumbnail_scale (Image& image, double xscale, double yscale, bool fixed = false);

#endif
