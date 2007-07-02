/*
 * Image Line
 * Copyright (C) 2007 Susanne Klaus, ExactCODE GmbH
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
void drawLine(Image& img, unsigned int x, unsigned int y, unsigned int x2, unsigned int y2,
	      const Image::iterator& color);
void drawRectange(Image& img, unsigned int x, unsigned int y, unsigned int x2, unsigned int y2,
		  const Image::iterator& color);
