/*
 * Canvas cropping.
 * Copyright (C) 2006 - 2011 René Rebe, ExactCODE
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

#ifndef __CROP_HH
#define __CROP_HH

void crop (Image& image, int x, int y, unsigned int w, unsigned int h);
void fastAutoCrop (Image& image);

#endif
