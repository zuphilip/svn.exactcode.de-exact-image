/*
 * Page boundary detection for auto-crop and de-skew.
 * Copyright (C) 2006 - 2008 René Reb, ExactCODE GmbH
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

#ifndef __DESKEW_HH
#define __DESKEW_HH

struct deskew_rect {
  double x, y, width, height, angle;
  double x_back, y_back;
};

deskew_rect deskewParameters (Image& image, int background_lines,
			      bool from_bottom = false);
bool deskew (Image& image, int background_lines, bool from_bottom = false);

#endif
