/*
 * Copyright (C) 2006 - 2008 René Rebe
 *           (C) 2006, 2007 Archivista GmbH, CH-8042 Zuerich
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <string>
#include <iostream>
#include <fstream>

#include <setjmp.h> // optional error recovery

#include "jpeg.hh"

#include "crop.hh"
#include "scale.hh"

/*
 * ERROR HANDLING:
 *
 * The JPEG library's standard error handler (jerror.c) is divided into
 * several "methods" which you can override individually.  This lets you
 * adjust the behavior without duplicating a lot of code, which you might
 * have to update with each future release.
 *
 * Our example here shows how to override the "error_exit" method so that
 * control is returned to the library's caller when a fatal error occurs,
 * rather than calling exit() as the standard error_exit method does.
 *
 * We use C's setjmp/longjmp facility to return control.  This means that the
 * routine which calls the JPEG library must first execute a setjmp() call to
 * establish the return point.  We want the replacement error_exit to do a
 * longjmp().  But we need to make the setjmp buffer accessible to the
 * error_exit routine.  To do this, we make a private extension of the
 * standard JPEG error handler object.
 *
 * Here's the extended error handler struct:
 */

struct my_error_mgr {
  struct jpeg_error_mgr pub;	/* "public" fields */

  jmp_buf setjmp_buffer;	/* for return to caller */
};

typedef struct my_error_mgr* my_error_ptr;

/*
 * Here's the routine that will replace the standard error_exit method:
 */

METHODDEF(void)
my_error_exit (j_common_ptr cinfo)
{
  /* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
  my_error_ptr myerr = (my_error_ptr) cinfo->err;

  /* Always display the message. */
  /* We could postpone this until after returning, if we chose. */
  (*cinfo->err->output_message) (cinfo);

  /* Return control to the setjmp point */
  longjmp(myerr->setjmp_buffer, 1);
}

void jpeg_compress_set_density (jpeg_compress_struct* dstinfo, const Image& image)
{
  dstinfo->JFIF_minor_version = 2; // emit JFIF 1.02 extension markers ...
  if (image.xres == 0 || image.yres == 0) {
    dstinfo->density_unit = 0; /* unknown */
    dstinfo->X_density = dstinfo->Y_density = 0;
  }
  else {
    dstinfo->density_unit = 1; /* 1 for dots/inch */
    dstinfo->X_density = image.xres;
    dstinfo->Y_density = image.yres;
  }
}

/* *** source manager *** */

typedef struct {
  struct jpeg_source_mgr pub;	/* public fields */

  std::istream* stream;
  JOCTET* buffer;		/* start of buffer */
  bool start_of_file;	/* have we gotten any data yet? */
} cpp_src_mgr;


#define INPUT_BUF_SIZE  4096	/* choose an efficiently fread'able size */

static void init_source (j_decompress_ptr cinfo)
{
  cpp_src_mgr* src = (cpp_src_mgr*) cinfo->src;
  src->start_of_file = true;
}

boolean fill_input_buffer (j_decompress_ptr cinfo)
{
  cpp_src_mgr* src = (cpp_src_mgr*) cinfo->src;
  
  size_t nbytes = src->stream->tellg ();

  src->stream->read ((char*)src->buffer, INPUT_BUF_SIZE);
  // if only a partial buffer was read, reset the state to be able
  // to get the new file position
  if (!*src->stream)
    src->stream->clear();
  nbytes = (size_t)src->stream->tellg () - nbytes;
  
  if (nbytes <= 0) {
    if (src->start_of_file)	/* Treat empty input file as fatal error */
      ERREXIT(cinfo, JERR_INPUT_EMPTY);
    WARNMS(cinfo, JWRN_JPEG_EOF);
    /* Insert a fake EOI marker */
    src->buffer[0] = (JOCTET) 0xFF;
    src->buffer[1] = (JOCTET) JPEG_EOI;
    nbytes = 2;
  }
  
  src->pub.next_input_byte = src->buffer;
  src->pub.bytes_in_buffer = nbytes;
  src->start_of_file = FALSE;
  
  return TRUE;
}


void skip_input_data (j_decompress_ptr cinfo, long num_bytes)
{
  cpp_src_mgr* src = (cpp_src_mgr*) cinfo->src;

  if (num_bytes > 0) {
    while (num_bytes > (long) src->pub.bytes_in_buffer) {
      num_bytes -= (long) src->pub.bytes_in_buffer;
      (void) fill_input_buffer(cinfo);
      /* note we assume that fill_input_buffer will never return FALSE,
       * so suspension need not be handled.
       */
    }
    src->pub.next_input_byte += (size_t) num_bytes;
    src->pub.bytes_in_buffer -= (size_t) num_bytes;
  }
}

void term_source (j_decompress_ptr cinfo)
{
  /* no work necessary here */
  free (((cpp_src_mgr*)cinfo->src)->buffer);
  free (cinfo->src);
}


void cpp_stream_src (j_decompress_ptr cinfo, std::istream* stream)
{
  cpp_src_mgr* src;

  if (cinfo->src == NULL) {	/* first time for this JPEG object? */
    cinfo->src = (jpeg_source_mgr*) malloc (sizeof(cpp_src_mgr));
    src = (cpp_src_mgr*) cinfo->src;
    src->buffer = (JOCTET *) malloc (INPUT_BUF_SIZE * sizeof(JOCTET));
  }

  src = (cpp_src_mgr*) cinfo->src;
  src->pub.init_source = init_source;
  src->pub.fill_input_buffer = fill_input_buffer;
  src->pub.skip_input_data = skip_input_data;
  src->pub.resync_to_restart = jpeg_resync_to_restart; /* use default method */
  src->pub.term_source = term_source;
  
  src->stream = stream;
  
  src->pub.bytes_in_buffer = 0; /* forces fill_input_buffer on first read */
  src->pub.next_input_byte = NULL; /* until buffer loaded */
}


/* *** destination manager *** */

typedef struct {
  struct jpeg_destination_mgr pub; /* public fields */

  std::ostream* stream;		/* target stream */
  JOCTET* buffer;		/* start of buffer */
} cpp_dest_mgr;

#define OUTPUT_BUF_SIZE  4096	/* choose an efficiently fwrite'able size */


void init_destination (j_compress_ptr cinfo)
{
  cpp_dest_mgr* dest = (cpp_dest_mgr*) cinfo->dest;

  /* Allocate the output buffer --- it will be released when done with image */
  dest->buffer = (JOCTET *)
      (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_IMAGE,
				  OUTPUT_BUF_SIZE * sizeof(JOCTET));

  dest->pub.next_output_byte = dest->buffer;
  dest->pub.free_in_buffer = OUTPUT_BUF_SIZE;
}

boolean empty_output_buffer (j_compress_ptr cinfo)
{
  cpp_dest_mgr* dest = (cpp_dest_mgr*) cinfo->dest;

  dest->stream->write ((char*)dest->buffer, OUTPUT_BUF_SIZE);
  if (!*dest->stream)
    ERREXIT(cinfo, JERR_FILE_WRITE);

  dest->pub.next_output_byte = dest->buffer;
  dest->pub.free_in_buffer = OUTPUT_BUF_SIZE;

  return TRUE;
}


void term_destination (j_compress_ptr cinfo)
{
  cpp_dest_mgr* dest = (cpp_dest_mgr*) cinfo->dest;
  size_t datacount = OUTPUT_BUF_SIZE - dest->pub.free_in_buffer;

  /* Write any data remaining in the buffer */
  if (datacount > 0) {
    dest->stream->write ((char*)dest->buffer, datacount);
    if (!*dest->stream)
      ERREXIT(cinfo, JERR_FILE_WRITE);
  }
  dest->stream->flush ();
  
  /* Make sure we wrote the output file OK */
  if (!*dest->stream)
    ERREXIT(cinfo, JERR_FILE_WRITE);
  
  free (cinfo->dest);
}


void cpp_stream_dest (j_compress_ptr cinfo, std::ostream* stream)
{
  cpp_dest_mgr* dest;
  
  /* first time for this JPEG object? */
  if (cinfo->dest == NULL) {
    cinfo->dest = (struct jpeg_destination_mgr *) malloc (sizeof(cpp_dest_mgr));
  }
  
  dest = (cpp_dest_mgr*) cinfo->dest;
  dest->pub.init_destination = init_destination;
  dest->pub.empty_output_buffer = empty_output_buffer;
  dest->pub.term_destination = term_destination;
  dest->stream = stream;
}

/* *** back on-topic *** */

JPEGCodec::JPEGCodec (Image* _image)
  : ImageCodec (_image)
{
}

bool JPEGCodec::readImage (std::istream* stream, Image& image)
{
  if (stream->peek () != 0xFF)
    return false;
  stream->get (); // consume silently
  if (stream->peek () != 0xD8)
    return false;
  
  if (0) { // TODO: differentiate JFIF vs. Exif?
    // quick magic check
      char buf [10];
      stream->read (buf, sizeof (buf));
      stream->seekg (0);
      
      if (buf[6] != 'J' || buf[7] != 'F' || buf[8] != 'I' || buf[9] != 'F')
	return false;
  }
  
  if (!readMeta (stream, image))
    return false;
  
  // on-demand compression
  image.setRawData (0);
  
  // freestanding instance
  JPEGCodec* codec = new JPEGCodec (&image);
  
  // private copy for deferred decoding
  stream->seekg (0);
  *stream >> codec->private_copy.rdbuf();
  
  image.setCodec (codec);
  
  return true;
}

bool JPEGCodec::writeImage (std::ostream* stream, Image& image, int quality,
			    const std::string& compress)
{
  std::string c (compress);
  std::transform (c.begin(), c.end(), c.begin(), tolower);

  // if the instance is freestanding it can only be called by the mux if the cache is valid
  if (_image && c != "recompress") {
    // since potentially meta data might have changed we have to re-create
    // the stream here
    doTransform (JXFORM_NONE, image, stream);
    std::cerr << "Wrote DCT coefficients" << std::endl;
    return true;
  }
  
  // really encode
  struct jpeg_compress_struct cinfo;
  struct jpeg_error_mgr jerr;

  JSAMPROW buffer[1];	/* pointer to JSAMPLE row[s] */

  /* Initialize the JPEG compression object with default error handling. */
  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_compress(&cinfo);
  cpp_stream_dest (&cinfo, stream);
  
  cinfo.in_color_space = JCS_UNKNOWN;
  if (image.bps == 8 && image.spp == 3)
    cinfo.in_color_space = JCS_RGB;
  else if (image.bps == 8 && image.spp == 1)
    cinfo.in_color_space = JCS_GRAYSCALE;
  else if (image.bps == 8 && image.spp == 4)
    cinfo.in_color_space = JCS_CMYK;

  if (cinfo.in_color_space == JCS_UNKNOWN) {
    if (image.bps < 8)
      std::cerr << "JPEGCodec: JPEG can not hold less than 8 bit-per-channel." << std::endl;
    else
      std::cerr << "Unhandled bps/spp combination." << std::endl;
    jpeg_destroy_compress(&cinfo);
    return false;
  }
  
  cinfo.image_width = image.w;
  cinfo.image_height = image.h;
  cinfo.input_components = image.spp;
  cinfo.data_precision = image.bps; 
  
  /* defaults depending on in_color_space */
  jpeg_set_defaults(&cinfo);
  
  jpeg_compress_set_density (&cinfo, image);

  jpeg_set_quality(&cinfo, quality, FALSE); /* do not limit to baseline-JPEG values */

  /* Start compressor */
  jpeg_start_compress(&cinfo, TRUE);

  /* Process data */
  while (cinfo.next_scanline < cinfo.image_height) {
    buffer[0] = (JSAMPLE*) image.getRawData() +
      cinfo.next_scanline*image.stride();
    (void) jpeg_write_scanlines(&cinfo, buffer, 1);
  }

  /* Finish compression and release memory */
  jpeg_finish_compress(&cinfo);
  jpeg_destroy_compress(&cinfo);

  if (jerr.num_warnings)
    std::cerr << jerr.num_warnings << " Warnings." << std::endl;

  return true;
}

// on-demand decoding
/*bool*/ void JPEGCodec::decodeNow (Image* image)
{
  // std::cerr << "JPEGCodec::decodeNow" << std::endl;
  
  // decode without scaling
  decodeNow (image, 1);
}

/*bool*/ void JPEGCodec::decodeNow (Image* image, int factor)
{
  struct jpeg_decompress_struct* cinfo = new jpeg_decompress_struct;
  struct my_error_mgr jerr;
  
  /* Step 1: allocate and initialize JPEG decompression object */

  /* We set up the normal JPEG error routines, then override error_exit. */
  cinfo->err = jpeg_std_error(&jerr.pub);
  jerr.pub.error_exit = my_error_exit;
  /* Establish the setjmp return context for my_error_exit to use. */
  if (setjmp(jerr.setjmp_buffer)) {
    /* If we get here, the JPEG code has signaled an error.
     * We need to clean up the JPEG object, close the input file, and return.
     */
    jpeg_destroy_decompress (cinfo);
    return;
  }
  
  jpeg_create_decompress (cinfo);
  
  /* Step 2: specify data source (eg, a file) */
  
  private_copy.seekg (0);
  cpp_stream_src (cinfo, &private_copy);

  /* Step 3: read file parameters with jpeg_read_header() */

  jpeg_read_header(cinfo, TRUE);
  
  /* Step 4: set parameters for decompression */
  
  cinfo->buffered_image = TRUE; /* select buffered-image mode */

  // TODO: set scaling
  if (factor != 1) {
    cinfo->scale_num = 1;
    cinfo->scale_denom = factor;
    cinfo->dct_method = JDCT_IFAST;
  }
  
  /* Step 5: Start decompressor */
  jpeg_start_decompress (cinfo);
  
  image->w = cinfo->output_width;
  image->h = cinfo->output_height;
  
  /* JSAMPLEs per row in output buffer */
  int row_stride = cinfo->output_width * cinfo->output_components;

  image->resize (image->w, image->h);
  
  /* Step 6: jpeg_read_scanlines(...); */
  
  uint8_t* data = image->getRawData ();
  JSAMPROW buffer[1];		/* pointer to JSAMPLE row[s] */
  
  while (! jpeg_input_complete(cinfo)) {
    jpeg_start_output(cinfo, cinfo->input_scan_number);
    while (cinfo->output_scanline < cinfo->output_height) {
      /* jpeg_read_scanlines expects an array of pointers to scanlines.
       * Here the array is only one element long, but you could ask for
       * more than one scanline at a time if that's more convenient.
       */
      buffer[0] = (JSAMPLE*) data+ (cinfo->output_scanline*row_stride);
      jpeg_read_scanlines(cinfo, buffer, 1);
    }
    jpeg_finish_output(cinfo);
  }

  jpeg_finish_decompress(cinfo);
  jpeg_destroy_decompress(cinfo);
  delete (cinfo);
  
  // shadow data is still valid for more transformations
  image->setCodec (this);
}

// in any case (we do not want artefacts): transformoption.trim = TRUE;

bool JPEGCodec::flipX (Image& image)
{
  doTransform (JXFORM_FLIP_H, image);
  return true;
}

bool JPEGCodec::flipY (Image& image)
{
  doTransform (JXFORM_FLIP_V, image);
  return true;
}

bool JPEGCodec::rotate (Image& image, double angle)
{
  // so roate if the first fraction is zero
  switch ((int)(angle * 10)) {
  case 900:  doTransform (JXFORM_ROT_90, image); return true;
  case 1800: doTransform (JXFORM_ROT_180, image); return true;
  case 2700: doTransform (JXFORM_ROT_270, image); return true;
  default:
    ; // no acceleration, fall thru
  }
  
  return false;
}

bool JPEGCodec::crop (Image& image, unsigned int x, unsigned int y, unsigned int w, unsigned int h)
{
  doTransform (JXFORM_NONE, image, 0 /* stream */, false /* to gray */, true /* crop */,
	       x, y, w, h);
  
  // reminder of 8x8 JPEG block crop
  x %= 8;
  y %= 8;
  if (x || y) {
    // invalidate, otherwise the ::crop() does call us again
    image.setRawData();
    // global crop, not our method
    ::crop(image, x, y, w, h);
  }
  
  return true;
}

bool JPEGCodec::toGray (Image& image)
{ 
  doTransform (JXFORM_NONE, image, 0 /* stream */, true /* to gray */);
  return true;
}

bool JPEGCodec::scale (Image& image, double xscale, double yscale)
{
  // we only support fast downscaling
  if (xscale > 1.0 || yscale > 1.0)
    return false; // let the generic scaler handle this
  
  int w_final = (int)(xscale * image.w);
  int h_final = (int)(xscale * image.h);

  std::cerr << "Scaling by partially loading DCT coefficients." << std::endl;
    
  // compute downscale factor
  int scale = (int) (xscale > yscale ? 1./xscale : 1./yscale);
  if      (scale > 8) scale = 8;
  else if (scale < 1) scale = 1;
  
  // we get values in the range [1,8] here, but libjpeg only
  // supports [1,2,4,8] - others are rounded down
  decodeNow (&image, scale);
  // due downscaling the private copy is no longer valid
  image.setRawData ();

  // TODO: test if we can just read the coefficients
  
  // we only have scaled in the range [1,2,4,8] and need to do the rest
  // manually
  xscale = (double)w_final / image.w;
  yscale = (double)h_final / image.h;
  
  if (xscale != 1.0 || yscale != 1.0)
    box_scale (image, xscale, yscale);
  
  return true;
}

bool JPEGCodec::readMeta (std::istream* stream, Image& image)
{
  stream->seekg (0);
  
  struct jpeg_decompress_struct* cinfo = new jpeg_decompress_struct;
  
  struct my_error_mgr jerr;
  
  /* Step 1: allocate and initialize JPEG decompression object */

  /* We set up the normal JPEG error routines, then override error_exit. */
  cinfo->err = jpeg_std_error(&jerr.pub);
  jerr.pub.error_exit = my_error_exit;
  /* Establish the setjmp return context for my_error_exit to use. */
  if (setjmp(jerr.setjmp_buffer)) {
    /* If we get here, the JPEG code has signaled an error.
     * We need to clean up the JPEG object, close the input file, and return.
     */
    jpeg_destroy_decompress (cinfo);
    free (cinfo);
    return false;
  }
  
  jpeg_create_decompress (cinfo);
  
  /* Step 2: specify data source (eg, a file) */
  
  cpp_stream_src (cinfo, stream);

  /* Step 3: read file parameters with jpeg_read_header() */

  jpeg_read_header(cinfo, TRUE);
  
  /* Step 4: set parameters for decompression */
  
  cinfo->buffered_image = TRUE; /* select buffered-image mode */
  
  /* Step 5: Start decompressor */
  
  jpeg_start_decompress (cinfo);
  
  image.w = cinfo->output_width;
  image.h = cinfo->output_height;
  image.spp = cinfo->output_components;
  image.bps = 8;
  
  /* These three values are not used by the JPEG code, merely copied */
  /* into the JFIF APP0 marker.  density_unit can be 0 for unknown, */
  /* 1 for dots/inch, or 2 for dots/cm.  Note that the pixel aspect */
  /* ratio is defined by X_density/Y_density even when density_unit=0. */
  switch (cinfo->density_unit)           /* JFIF code for pixel size units */
    {
    case 1:
      image.xres = cinfo->X_density;
      image.yres = cinfo->Y_density;
      break;
    case 2:
      image.xres = cinfo->X_density * 254 / 100;
      image.yres = cinfo->Y_density * 254 / 100;
      break;
    default:
      image.xres = image.yres = 0;
    }
  
  /* Step 8: Release JPEG decompression object */
  
  /* This is an important step since it will release a good deal of memory. */
  jpeg_finish_decompress(cinfo);
  jpeg_destroy_decompress (cinfo);
  delete (cinfo);

  return true;
}

bool JPEGCodec::doTransform (JXFORM_CODE code, Image& image,
			     std::ostream* s, bool to_gray, bool crop,
			     unsigned int x, unsigned int y,
			     unsigned int w, unsigned int h)
{
  jpeg_transform_info transformoption; /* image transformation options */
  
  jpeg_decompress_struct srcinfo;
  jpeg_compress_struct dstinfo;
  jpeg_error_mgr jsrcerr, jdsterr;
  
  std::stringstream stream;
  
  std::cerr << "Transforming DCT coefficients." << std::endl;
  
  jvirt_barray_ptr * src_coef_arrays;
  jvirt_barray_ptr * dst_coef_arrays;
  
  /* Initialize the JPEG decompression object with default error handling. */
  srcinfo.err = jpeg_std_error(&jsrcerr);
  jpeg_create_decompress(&srcinfo);
  /* Initialize the JPEG compression object with default error handling. */
  dstinfo.err = jpeg_std_error(&jdsterr);
  /* Initialize the JPEG compression object with default error handling. */
  jpeg_create_compress(&dstinfo);
  
  srcinfo.mem->max_memory_to_use = dstinfo.mem->max_memory_to_use;
  
  private_copy.seekg (0);
  cpp_stream_src (&srcinfo, &private_copy);
  
  /* Read file header */
  jpeg_read_header(&srcinfo, TRUE);
  
  transformoption.transform = code;
  transformoption.trim = TRUE;
  transformoption.perfect = FALSE;
  transformoption.force_grayscale = to_gray ? TRUE : FALSE;
  
  transformoption.crop = crop ? TRUE : FALSE;
  if (crop) {
    transformoption.crop_xoffset = x;
    transformoption.crop_xoffset_set = JCROP_POS;
    transformoption.crop_yoffset = y;
    transformoption.crop_yoffset_set = JCROP_POS;
    transformoption.crop_width = w;
    transformoption.crop_width_set = JCROP_POS;
    transformoption.crop_height = h;
    transformoption.crop_height_set = JCROP_POS;
  }
  
  /* Any space needed by a transform option must be requested before
   * jpeg_read_coefficients so that memory allocation will be done right.
   */
  jtransform_request_workspace(&srcinfo, &transformoption);
  
  /* Read source file as DCT coefficients */
  src_coef_arrays = jpeg_read_coefficients(&srcinfo);
  
  /* Initialize destination compression parameters from source values */
  jpeg_copy_critical_parameters(&srcinfo, &dstinfo);

  /* Adjust destination parameters if required by transform options;
   * also find out which set of coefficient arrays will hold the output.
   */
  if (transformoption.transform != JXFORM_NONE ||
      transformoption.force_grayscale ||
      transformoption.crop)
    dst_coef_arrays = jtransform_adjust_parameters(&srcinfo, &dstinfo,
						   src_coef_arrays,
						   &transformoption);
  else
    dst_coef_arrays = src_coef_arrays;
  
  /* Specify data destination for compression */
  cpp_stream_dest (&dstinfo, s ? s : &stream);
  
  // as we read back just the basics, some manual translations
  switch (code) {
  case JXFORM_ROT_90:
  case JXFORM_ROT_270:
    { int t = image.xres; image.xres = image.yres; image.yres = t;} break;
  default:
    ; // silence compiler
  }
  
  jpeg_compress_set_density (&dstinfo, image);
  
  /* Start compressor (note no image data is actually written here) */
  jpeg_write_coefficients(&dstinfo, dst_coef_arrays);
  
  /* Execute image transformation, if any */
  jtransform_execute_transformation(&srcinfo, &dstinfo,
				    src_coef_arrays,
				    &transformoption);
  
  /* Finish compression and release memory */
  jpeg_finish_compress(&dstinfo);
  jpeg_destroy_compress(&dstinfo);
  jpeg_finish_decompress(&srcinfo);
  jpeg_destroy_decompress(&srcinfo);
  
  // if we are not just writing
  if (!s) {
    private_copy.str (stream.str());
    // copy into the shadow buffer
    
    // Update meta, w/h,spp might have changed.
    // We re-read the header because we do not want to re-hardcode the
    // trimming required for some operations.
    readMeta (&private_copy, image);

    // if the data is accessed again, it must be re-encoded
    image.setRawData(0);
    image.setCodec(this);
  }
  
  return true;
}

JPEGCodec jpeg_loader;
