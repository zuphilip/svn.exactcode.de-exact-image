/*
 * Copyright (C) 2005 - 2013 René Rebe
 *           (C) 2005 - 2007 Archivista GmbH, CH-8042 Zuerich
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


// quickly counts pixels, returns whether set pixels are below
// threshold and optionally can return the set pixels of all pixels

// if the image is not 1 bit per pixel it will be optimized to b/w
// (if you want more control over that call it yourself before)
// the margin are the border pixels skipped, it must be a multiple
// of 8 for speed reasons and will be rounded down to the next
// multiple of 8 if necessary.

#include "Image.hh"

bool detect_empty_page (Image& image, double percent = 0.05,
			int marginH = 8, int marginV = 16,
			int* set_pixels = 0);
