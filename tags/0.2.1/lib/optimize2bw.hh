
/*
 * Copyright (C) 2005 René Rebe
 *           (C) 2005 Archivista GmbH, CH-8042 Zuerich
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

#include "Image.hh"

// optimizes the image for b/w images
// it does not do the thresholding, the result is still 8 bit per pixel
// so the caller can scale on the shaded data

void optimize2bw (Image& image, int low = 0, int high = 0,
		  int sloppy_threshold = 0,
		  int radius = 3, double standard_deviation = 2.1);
