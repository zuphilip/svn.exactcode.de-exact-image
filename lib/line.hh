/*
 * Image Line
 * Copyright (C) 2007 Valentin Ziegler and René Rebe
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


#include <vector>
#include "Image.hh"

class Line 
{
public:

  Line();
  ~Line();

  void drawLine(Image& img, unsigned int a_start, unsigned int a_end, unsigned int b, bool horizontal);

  void drawHLine(Image& img, unsigned int x_start, unsigned int x_end, unsigned int y);

  void drawVLine(Image& img, unsigned int y_start, unsigned int y_end, unsigned int x);

};
