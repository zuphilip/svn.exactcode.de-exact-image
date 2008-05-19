/*
 * Copyright (C) 2007 Valentin Ziegler, ExactCODE GmbH Germany.
 *               2008 Rene Rebe, ExactCODE GmbH Germany.
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

#include "FG-Matrix.hh"
#include <vector>

// "outline" contours
class Contours
{
public:
  typedef std::vector < std::pair<unsigned int, unsigned int> > Contour;
  typedef DataMatrix<int> VisitMap;

  Contours(const FGMatrix& image);
  Contours() {} // empty constructor for generic usage

  ~Contours();

  std::vector <Contour*> contours;
};


// "mindpoint of scanlines" inner storke /tracer/"
class MidContours : public Contours
{
public:
  MidContours(const FGMatrix& image);
};

// "local maxima of neighbor distance to border"
class InnerContours : public Contours
{
  typedef enum {LEFT, RIGHT, UP, DOWN} dir;
  
  unsigned int RecursiveDist(const FGMatrix& image,
			     int x, int y,
			     dir d, unsigned int r);
  
public:
  InnerContours(const FGMatrix& image);
};
