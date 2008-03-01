
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

#define fread(mem,n,m,stream) wrapped_fread (stream, (char*)mem,n*m)
#define fwrite(mem,n,m,stream) wrapped_fwrite (stream, (char*)mem,n*m)

#define fgetc(stream) stream->get ()
#define fgets(mem,n,stream) stream->get ((char*)mem, n);
#define putc(c,stream) stream->put (c)

#define tmpfile new std::stringstream
#define fclose(stream) delete (stream);

#include "dcraw.h"

bool DCRAWCodec::readImage (std::istream* stream, Image& im)
{
#ifndef NO_LCMS
  char *cam_profile = NULL, *out_profile = NULL;
#endif
  
  std::iostream ios (stream->rdbuf());
  ifp = &ios;
  
  identify();
  
  if (!is_raw)
    return false;
  
  if (load_raw == kodak_ycbcr_load_raw) {
    height += height & 1;
    width  += width  & 1;
  }
  
  int half_size = 0, quality, use_fuji_rotate=1;
  shrink = half_size && filters;
  iheight = (height + shrink) >> shrink;
  iwidth  = (width  + shrink) >> shrink;
   
  image = (ushort (*)[4]) calloc (iheight*iwidth*sizeof *image + meta_length, 1);
  merror (image, "main()");
  meta_data = (char *) (image + iheight*iwidth);
  
  fseek (ifp, data_offset, SEEK_SET);
  (*load_raw)();
  // bad_pixels();
  
  height = iheight;
  width  = iwidth;
  quality = 2 + !fuji_width;
  
  if (is_foveon && !document_mode) foveon_interpolate();
  if (!is_foveon && document_mode < 2) scale_colors();
  if (shrink) filters = 0;
  cam_to_cielab (NULL,NULL);
  if (filters && !document_mode) {
    if (quality == 0)
      lin_interpolate();
    else if (quality < 3 || colors > 3)
      vng_interpolate();
    else ahd_interpolate();
  }
  if (sigma_d > 0 && sigma_r > 0) bilateral_filter();
  if (use_fuji_rotate) fuji_rotate();
#ifndef NO_LCMS
  if (cam_profile) apply_profile (cam_profile, out_profile);
#endif
  convert_to_rgb();
  
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
    
  return true;
}

bool DCRAWCodec::writeImage (std::ostream* stream, Image& image,
			     int quality, const std::string& compress)
{
  return false;
}

DCRAWCodec dcraw_loader;
