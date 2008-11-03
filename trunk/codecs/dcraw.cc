/*
 * Copyright (C) 2006 - 2008 René Rebe
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

#include "dcraw.hh"
#include <istream>
#include <sstream>

// now this is an macro conversion from C FILE* to C++ iostream ,-)

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define FILE std::iostream

#undef SEEK_SET
#undef SEEK_CUR
#undef SEEK_END

#define SEEK_SET std::ios::beg
#define SEEK_CUR std::ios::cur
#define SEEK_END std::ios::end

#define fseek(stream,pos,kind) stream->seekg (pos, kind)
#define ftell(stream) (int) stream->tellg ()

static inline int wrapped_fread (std::iostream* stream, char* mem, int n)
{
	return stream->read (mem, n) ? n : 0;
}

static inline int wrapped_fwrite (std::iostream* stream, char* mem, int n)
{
	stream->write (mem,n);
	return stream->good() ? n : 0;
}

static inline int wrapped_fscanf (std::iostream* stream, const char* buf, ...)
{
  std::cerr << "TODO: " << __PRETTY_FUNCTION__ << std::endl;
  // TODO: so far only %d and %f are used
  return 0;
}

#define fread(mem,n,m,stream) wrapped_fread (stream, (char*)mem,n*m)
#define fwrite(mem,n,m,stream) wrapped_fwrite (stream, (char*)mem,n*m)

#define fscanf(stream,buf,...) wrapped_fscanf (stream, buf, __VA_ARGS__)

#define fgetc(stream) stream->get ()
#define getc(stream) stream->get ()
#define fgets(mem,n,stream) stream->get ((char*)mem, n);
#define putc(c,stream) stream->put (c)

#define tmpfile new std::stringstream
#define fclose(stream) delete (stream);

#define feof(stream) stream->eof()
#define ftello(stream) stream->tellg()
#define fseeko(stream, pos, mode) stream->seekg(pos) // TODO: mode

// now include the original dcraw source with our translation macros in-place

#include "dcraw.h"

bool DCRAWCodec::readImage (std::istream* stream, Image& im)
{
#ifndef NO_LCMS
  char *cam_profile = NULL, *out_profile = NULL;
#endif
  
  std::iostream ios (stream->rdbuf());
  ifp = &ios;
  
  // dcraw::main()
  
  int use_fuji_rotate=1, quality, i;
  
#ifndef NO_LCMS
  char *cam_profile=0, *out_profile=0;
#endif

  if (use_camera_matrix < 0)
      use_camera_matrix = use_camera_wb;
  
  identify();
  
  if (!is_raw)
    return false;
  
  if (load_raw == &CLASS kodak_ycbcr_load_raw) {
    height += height & 1;
    width  += width  & 1;
  }
  
  shrink = filters &&
    (half_size || threshold || aber[0] != 1 || aber[2] != 1);
  iheight = (height + shrink) >> shrink;
  iwidth  = (width  + shrink) >> shrink;
  
  if (use_camera_matrix && cmatrix[0][0] > 0.25) {
    memcpy (rgb_cam, cmatrix, sizeof cmatrix);
    raw_color = 0;
  }
  
  image = (ushort (*)[4]) calloc (iheight*iwidth, sizeof *image);
  
  if (meta_length) {
    meta_data = (char *) malloc (meta_length);
    merror (meta_data, "main()");
  }
  
  if (shot_select >= is_raw)
    fprintf (stderr,_("%s: \"-s %d\" requests a nonexistent image!\n"),
	     ifname, shot_select);
  fseeko (ifp, data_offset, SEEK_SET);
  (*load_raw)();
  // bad_pixels();
  if (zero_is_bad) remove_zeroes();
  
  quality = 2 + !fuji_width;
  
  if (is_foveon && !document_mode) foveon_interpolate();
  if (!is_foveon && document_mode < 2) scale_colors();
  pre_interpolate();
  if (filters && !document_mode) {
    if (quality == 0)
      lin_interpolate();
    else if (quality == 1 || colors > 3)
      vng_interpolate();
    else if (quality == 2)
      ppg_interpolate();
    else ahd_interpolate();
  }
  if (mix_green)
    for (colors=3, i=0; i < height*width; i++)
      image[i][1] = (image[i][1] + image[i][3]) >> 1;
  if (!is_foveon && colors == 3) median_filter();
  if (!is_foveon && highlight == 2) blend_highlights();
  if (!is_foveon && highlight > 2) recover_highlights();
  if (use_fuji_rotate) fuji_rotate();
#ifndef NO_LCMS
  if (cam_profile) apply_profile (cam_profile, out_profile);
#endif
  convert_to_rgb();
  if (use_fuji_rotate) stretch();
  
  im.bps = 16;
  im.spp = 3;
  im.resize(width, height);
  
  // the non-linear gamma by default
  uint16_t lut [0x10000];
  const double gamma = 1.8;
  const double one_over_gamma = 1. / gamma;
  for (int i = 0; i < 0x10000; ++i)
    lut [i] = pow( (double) i / 0xFFFF, one_over_gamma) * 0xFFFF;
  
  uint16_t* ptr = (uint16_t*) im.getRawData();
  for (int row = 0; row < height; ++row)
    for (int col = 0; col < width; ++col)
      for (int c = 0; c < colors; ++c)
	*ptr++ = lut[ image[row*width+col][c] ];
  
 cleanup:
  if (meta_data) free (meta_data);
  if (oprof) free (oprof);
  if (image) free (image);
  
  return true;
}

bool DCRAWCodec::writeImage (std::ostream* stream, Image& image,
			     int quality, const std::string& compress)
{
  return false;
}

DCRAWCodec dcraw_loader;
