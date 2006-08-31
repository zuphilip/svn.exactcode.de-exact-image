
#include <endian.h>
#include <stdlib.h>

#include <png.h>

#include "png.hh"

#include <iostream>

#if 0
#define PNG_BYTES_TO_CHECK 4
int check_if_png(char *file_name, FILE **fp)
{
   char buf[PNG_BYTES_TO_CHECK];

   /* Open the prospective PNG file. */
   if ((*fp = fopen(file_name, "rb")) == NULL)
      return 0;

   /* Read in some of the signature bytes */
   if (fread(buf, 1, PNG_BYTES_TO_CHECK, *fp) != PNG_BYTES_TO_CHECK)
      return 0;

   /* Compare the first PNG_BYTES_TO_CHECK bytes of the signature.
      Return nonzero (true) if they match */

   return(!png_sig_cmp(buf, (png_size_t)0, PNG_BYTES_TO_CHECK));
}
#endif

bool PNGCodec::readImage (std::istream* stream, Image& image)
{
  png_structp png_ptr;
  png_infop info_ptr;
  png_uint_32 width, height;
  int bit_depth, color_type, interlace_type;
  
  png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
				   NULL /*user_error_ptr*/,
				   NULL /*user_error_fn*/,
				   NULL /*user_warning_fn*/);
  
  if (png_ptr == NULL)
    return 0;
  
  /* Allocate/initialize the memory for image information.  REQUIRED. */
  info_ptr = png_create_info_struct(png_ptr);
  if (info_ptr == NULL) {
    png_destroy_read_struct(&png_ptr, png_infopp_NULL, png_infopp_NULL);
    return 0;
  }
  
  /* Set error handling if you are using the setjmp/longjmp method (this is
   * the normal method of doing things with libpng).  REQUIRED unless you
   * set up your own error handlers in the png_create_read_struct() earlier.
   */
  
  if (setjmp(png_jmpbuf(png_ptr))) {
    /* Free all of the memory associated with the png_ptr and info_ptr */
    png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);
    /* If we get here, we had a problem reading the file */
    return 0;
  }
  
  /* Set up the input control if you are using standard C streams */
  png_init_io(png_ptr, file);
  
  ///* If we have already read some of the signature */
  //png_set_sig_bytes(png_ptr, sig_read);
  
  /* The call to png_read_info() gives us all of the information from the
   * PNG file before the first IDAT (image data chunk).  REQUIRED
   */
  png_read_info (png_ptr, info_ptr);
  
  png_get_IHDR (png_ptr, info_ptr, &width, &height, &bit_depth, &color_type,
		&interlace_type, int_p_NULL, int_p_NULL);
  
  image.w = width;
  image.h = height;
  image.bps = bit_depth;
  image.spp = info_ptr->channels;
  
  png_uint_32 res_x, res_y;
  res_x = png_get_x_pixels_per_meter(png_ptr, info_ptr);
  res_y = png_get_y_pixels_per_meter(png_ptr, info_ptr);
  image.xres = (int)((2.54*res_x+.5) / 100);
  image.yres = (int)((2.54*res_y+.5) / 100);
  
  /* Extract multiple pixels with bit depths of 1, 2, and 4 from a single
   * byte into separate bytes (useful for paletted and grayscale images) */
  // png_set_packing(png_ptr);

  /* Change the order of packed pixels to least significant bit first
   * (not useful if you are using png_set_packing). */
  // png_set_packswap(png_ptr);

  /* Expand paletted colors into true RGB triplets */
  if (color_type == PNG_COLOR_TYPE_PALETTE) {
    png_set_palette_to_rgb(png_ptr);
    image.bps = 8;
    if (info_ptr->num_trans)
      image.spp = 4;
    else
      image.spp = 3;
  }
  
#if 0 // no longer needed
  /* Expand grayscale images to the full 8 bits from 2, or 4 bits/pixel */
  if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth > 1 && bit_depth < 8) {
    png_set_gray_1_2_4_to_8(png_ptr);
    image.bps = 8;
  }
#endif  
  
  /* Expand paletted or RGB images with transparency to full alpha channels
   * so the data will be available as RGBA quartets.
   */
  if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
    png_set_tRNS_to_alpha(png_ptr);
  
  /* Set the background color to draw transparent and alpha images over.
   * It is possible to set the red, green, and blue components directly
   * for paletted images instead of supplying a palette index.  Note that
   * even if the PNG file supplies a background, you are not required to
   * use it - you should use the (solid) application background if it has one.
   */
#if 0
  png_color_16* image_background;
  if (png_get_bKGD(png_ptr, info_ptr, &image_background)) {
    png_set_background(png_ptr, image_background,
		       PNG_BACKGROUND_GAMMA_FILE, 1, 1.0);
    image.spp = 3;
  }
#endif
  
  /* If you want to shift the pixel values from the range [0,255] or
   * [0,65535] to the original [0,7] or [0,31], or whatever range the
   * colors were originally in:
   */
  if (png_get_valid(png_ptr, info_ptr, PNG_INFO_sBIT)) {
    png_color_8p sig_bit;
    
    png_get_sBIT(png_ptr, info_ptr, &sig_bit);
    png_set_shift(png_ptr, sig_bit);
  }
  
#if __BYTE_ORDER != __BIG_ENDIAN
  /* swap bytes of 16 bit files to least significant byte first
     we store them in CPU byte order in memory */
  png_set_swap(png_ptr);
#endif

  /* Turn on interlace handling.  REQURIED if you are not using
   * png_read_image().  To see how to handle interlacing passes,
   * see the png_read_row() method below:
   */
  int number_passes = png_set_interlace_handling (png_ptr);
  
  /* Optional call to gamma correct and add the background to the palette
   * and update info structure.  REQUIRED if you are expecting libpng to
   * update the palette for you (ie you selected such a transform above).
   */
  png_read_update_info(png_ptr, info_ptr);

  /* Allocate the memory to hold the image using the fields of info_ptr. */
  int stride = png_get_rowbytes (png_ptr, info_ptr);
  
  image.data = (unsigned char*) malloc (stride * height);
  png_bytep row_pointers[1];
  
  /* The other way to read images - deal with interlacing: */
  for (int pass = 0; pass < number_passes; ++pass)
    for (unsigned int y = 0; y < height; ++y) {
      row_pointers[0] = image.data + y * stride;
      png_read_rows(png_ptr, row_pointers, png_bytepp_NULL, 1);
    }
  
  /* clean up after the read, and free any memory allocated - REQUIRED */
  png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);
  
  /* that's it */
  return true;
}

bool PNGCodec::writeImage (std::ostream* stream, Image& image, int quality,
			   const std::string& compress)
{
  png_structp png_ptr;
  png_infop info_ptr;
  
  png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
				    NULL /*user_error_ptr*/,
				    NULL /*user_error_fn*/,
				    NULL /*user_warning_fn*/);
  
  if (png_ptr == NULL) {
    return false;
  }
  
  /* Allocate/initialize the memory for image information.  REQUIRED. */
  info_ptr = png_create_info_struct(png_ptr);
  if (info_ptr == NULL) {
    png_destroy_write_struct(&png_ptr, png_infopp_NULL);
    return false;
  }
  
  /* Set error handling if you are using the setjmp/longjmp method (this is
   * the normal method of doing things with libpng).  REQUIRED unless you
   * set up your own error handlers in the png_create_read_struct() earlier.
   */
  
  if (setjmp(png_jmpbuf(png_ptr))) {
    /* Free all of the memory associated with the png_ptr and info_ptr */
    png_destroy_write_struct(&png_ptr, &info_ptr);
    /* If we get here, we had a problem reading the file */
    return false;
  }
  
  png_info_init (info_ptr);
  
  /* Set up the input control if you are using standard C streams */
  png_init_io(png_ptr, file);
  
  ///* If we have already read some of the signature */
  //png_set_sig_bytes(png_ptr, sig_read);
  
  int color_type;
  switch (image.spp) {
  case 1:
    color_type = PNG_COLOR_TYPE_GRAY;
    break;
  case 4:
    color_type = PNG_COLOR_TYPE_RGB_ALPHA;
    break;
  default:
    color_type = PNG_COLOR_TYPE_RGB;
  }
  
  png_set_IHDR (png_ptr, info_ptr, image.w, image.h, image.bps, color_type,
		PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
		PNG_FILTER_TYPE_BASE);
  
  png_set_pHYs (png_ptr, info_ptr,
		(int)(image.xres*100/2.54), (int)(image.yres*100/2.54),
		PNG_RESOLUTION_METER);

  /* The call to png_read_info() gives us all of the information from the
   * PNG file before the first IDAT (image data chunk).  REQUIRED
   */
  png_write_info (png_ptr, info_ptr);
  
  int stride = png_get_rowbytes (png_ptr, info_ptr);
  
  png_bytep row_pointers[1]; 
  /* The other way to write images */
  int number_passes = 1;
  for (int pass = 0; pass < number_passes; ++pass)
    for (int y = 0; y < image.h; ++y) {
      row_pointers[0] = image.data + y * stride;
      png_write_rows(png_ptr, (png_byte**)&row_pointers, 1);
    }

  png_write_end(png_ptr, NULL);
  
  /* clean up after the read, and free any memory allocated - REQUIRED */
  png_destroy_write_struct(&png_ptr, &info_ptr);
  
  return true;
}

PNGCodec png_loader;
