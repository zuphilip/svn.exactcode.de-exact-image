/*
 * A Balanced Dithering Technique
 * Copyright (C) 2006 - 2013 René Rebe, ExactCOD GmbH Germany
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
 *
 * Based uppon:
 *   C/C++ Users Journal, December 1998: Thiadmer Riemersma
 */

#include "riemersma.h"

#include <math.h>
#include <string.h>

// substitutes "log2(n)", which is apparently not available on BSD, OS X
static inline double priv_log2(double n) {
    return log(n) / log(2);
}

enum direction_t {
  NONE,
  UP,
  LEFT,
  DOWN,
  RIGHT,
};

// variables needed for the Riemersma dither algorithm
static int cur_x = 0, cur_y = 0;
static int img_width = 0, img_height = 0;
static int img_bytes = 0;
static float img_factor;
static uint8_t* img_ptr;

#define SIZE 16                 // queue size: number of pixels remembered
#define MAX  16                 // relative weight of youngest pixel in the
                                // queue, versus the oldest pixel

static int weights[SIZE];       // weights for the errors of recent pixels

static void init_weights(int a[], int size, int max)
{
  double m = exp(log(max) / (size-1));
  double v;
  int i;

  for (i = 0, v = 1.0; i < size; i++) {
    a[i] = (int)(v + 0.5);  // store rounded value
    v *= m;                 // next value
  }
}

static void dither_pixel(uint8_t *pixel)
{
  static int error[SIZE]; // queue with error values of recent pixels
 int err = 0L;
 for (int i = 0; i < SIZE; i++)
    err += error[i] * weights[i];

 float pvalue = *pixel + err / MAX;

  pvalue = floor (pvalue * img_factor + 0.5) / img_factor;
  if (pvalue > 255)
    pvalue = 255;
  else if (pvalue < 0)
    pvalue = 0;

  memmove(error, error + 1, (SIZE - 1) * sizeof error[0]);    // shift queue
  error[SIZE - 1] = *pixel - (uint8_t)(pvalue + 0.5);
  *pixel = (uint8_t)(pvalue + 0.5);
}

static void move(direction_t direction)
{
  // dither the current pixel
  if (cur_x >= 0 && cur_x < img_width &&
      cur_y >= 0 && cur_y < img_height)
    dither_pixel(img_ptr);

  // move to the next pixel
  switch (direction) {
  case LEFT:
    --cur_x;
    img_ptr -= img_bytes;
    break;
  case RIGHT:
    ++cur_x;
    img_ptr += img_bytes;
    break;
  case UP:
    --cur_y;
    img_ptr -= img_width * img_bytes;
    break;
  case DOWN:
    ++cur_y;
    img_ptr += img_width * img_bytes;
    break;
  }
}

void hilbert_level(int level, direction_t direction)
{
  if (level == 1) {
    switch (direction) {
    case LEFT:
      move(RIGHT);
      move(DOWN);
      move(LEFT);
      break;
    case RIGHT:
      move(LEFT);
      move(UP);
      move(RIGHT);
      break;
    case UP:
      move(DOWN);
      move(RIGHT);
      move(UP);
      break;
    case DOWN:
      move(UP);
      move(LEFT);
      move(DOWN);
      break;
    }
  }
  else {
    switch (direction) {
    case LEFT:
      hilbert_level(level - 1, UP);
      move(RIGHT);
      hilbert_level(level - 1, LEFT);
      move(DOWN);
      hilbert_level(level - 1, LEFT);
      move(LEFT);
      hilbert_level(level - 1, DOWN);
      break;
    case RIGHT:
      hilbert_level(level - 1, DOWN);
      move(LEFT);
      hilbert_level(level - 1, RIGHT);
      move(UP);
      hilbert_level(level - 1, RIGHT);
      move(RIGHT);
      hilbert_level(level - 1, UP);
      break;
    case UP:
      hilbert_level(level - 1, LEFT);
      move(DOWN);
      hilbert_level(level - 1, UP);
      move(RIGHT);
      hilbert_level(level - 1, UP);
      move(UP);
      hilbert_level(level - 1, RIGHT);
      break;
    case DOWN:
      hilbert_level(level - 1, RIGHT);
      move(UP);
      hilbert_level(level - 1, DOWN);
      move(LEFT);
      hilbert_level(level - 1, DOWN);
      move(DOWN);
      hilbert_level(level - 1, LEFT);
      break;
    }
  }
}


void Riemersma(Image& image, int shades)
{
  uint8_t* raw = image.getRawData();
  img_width = image.w;
  img_height = image.h;
  img_bytes = image.spp;
  
  // determine the required order of the Hilbert curve
  const int size = img_width > img_height ? img_width : img_height;
  
  for (int ch = 0; ch < img_bytes; ++ch) {
    int level = (int) priv_log2 (size);
    if ((1L << level) < size)
      ++level;
    
    init_weights (weights, SIZE,MAX);
    img_factor = (-1.0 + shades) / 255.0;
    
    cur_x = 0;
    cur_y = 0;
    
    img_ptr = raw + ch;
    
    if (level > 0)
      hilbert_level(level, UP);
    
    move(NONE);
  }
}
