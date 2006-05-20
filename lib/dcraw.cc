
#include "dcraw.hh"

#include "dcraw.h"

bool DCRAWLoader::readImage (FILE* file, Image& im)
{
#ifndef NO_LCMS
  char *cam_profile = NULL, *out_profile = NULL;
#endif
  
  ifp = file;
  
  int status = (identify(),!is_raw);
  
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
  bad_pixels();
  
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
  im.New(width, height);
  
  uint16_t* ptr = (uint16_t*) im.data;
  for (int row = 0; row < height; ++row)
    for (int col = 0; col < width; ++col)
      for (int c = 0; c < colors; ++c)
	*ptr++ = image[row*width+col][c];
    
  
  return true;
}

bool DCRAWLoader::writeImage (FILE* file, Image& image,
			    int quality, const std::string& compress)
{
  return false;
}

DCRAWLoader dcraw_loader;
