/*
 * Copyright (C) 2006, 2007 René Rebe
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
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <string>
#include <iostream>
#include <fstream>

/*
 * <setjmp.h> is used for the optional error recovery mechanism
 */

#include <setjmp.h>

#include "jpeg.hh"

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

JPEGCodec::~JPEGCodec ()
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
  
  // private copy for deferred decoding
  stream->seekg (0);
  *stream >> private_copy.rdbuf();
  
  image.setCodec (this);
  
  return true;
}

bool JPEGCodec::writeImage (std::ostream* stream, Image& image, int quality,
			    const std::string& compress)
{
  // write original DCT coefficients ???
  if (!image.isModified() && image.getCodec () && image.getCodec()->getID() == getID())
    {
      std::cerr << "Shadow image data valid: writing orig DCT" << std::endl;
      
      // TODO: hm - ok it is not that easy, meta might changed, redo ...
      
      stream->write (private_copy.str().c_str(), private_copy.str().size());
      return true;
    }
  
  // really encode
  
  std::cerr << "Shadow image data modifed, classic compress." << std::endl;
  
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
  
  cinfo.JFIF_minor_version = 2; // emit JFIF 1.02 extension markers ...
  if (image.xres == 0 || image.yres == 0) {
    cinfo.density_unit = 0; /* unknown */
    cinfo.X_density = cinfo.Y_density = 0;
  }
  else {
    cinfo.density_unit = 1; /* 1 for dots/inch */
    cinfo.X_density = image.xres;
    cinfo.Y_density = image.yres;
  }
  jpeg_set_quality(&cinfo, quality, FALSE); /* do not limit to baseline-JPEG values */

  /* Start compressor */
  jpeg_start_compress(&cinfo, TRUE);

  /* Process data */
  while (cinfo.next_scanline < cinfo.image_height) {
    buffer[0] = (JSAMPLE*) image.getRawData() +
      cinfo.next_scanline*image.Stride();
    (void) jpeg_write_scanlines(&cinfo, buffer, 1);
  }

  /* Finish compression and release memory */
  jpeg_finish_compress(&cinfo);
  jpeg_destroy_compress(&cinfo);

  if (jerr.num_warnings)
    std::cout << jerr.num_warnings << " Warnings." << std::endl;

  return true;
}

// on-demand decoding
/*bool*/ void JPEGCodec::decodeNow (Image* image)
{
  std::cerr << "JPEGCodec::decodeNow" << std::endl;
  
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
  
  /* Step 5: Start decompressor */
  jpeg_start_decompress (cinfo);
  
  /* JSAMPLEs per row in output buffer */
  int row_stride = cinfo->output_width * cinfo->output_components;

  image->New (image->w, image->h);
  
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
  
  // shadow data is still valid for more transformations
  image->setCodec (this);
}

// in any case (we do not want artefacts): transformoption.trim = TRUE;

bool JPEGCodec::flipX (Image& image)
{
  std::cerr << "JPEGCodec::flipX" << std::endl;
  do_transform (JXFORM_FLIP_H, image);
  return true;
}

bool JPEGCodec::flipY (Image& image)
{
  std::cerr << "JPEGCodec::flipY" << std::endl;
  do_transform (JXFORM_FLIP_V, image);
  return true;
}

bool JPEGCodec::rotate (Image& image, double angle)
{
  std::cerr << "JPEGCodec::rotate" << std::endl;
  
  if (angle == 90) 
    { do_transform (JXFORM_ROT_90, image); return true; }
  if (angle == 180)
    { do_transform (JXFORM_ROT_180, image); return true; }
  if (angle == 270)
    { do_transform (JXFORM_ROT_270, image); return true; }

  // no acceleration, fall thru
  return false;
}

// TODO?: transformoption.force_grayscale = FALSE;

bool JPEGCodec::scale (Image& image, double xscale, double yscale)
{
  return false; // TODO: look into epeg and implement
}

bool JPEGCodec::readMeta (std::istream* stream, Image& image, bool just_basic)
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
  
  if (!just_basic) {
    /* These three values are not used by the JPEG code, merely copied */
    /* into the JFIF APP0 marker.  density_unit can be 0 for unknown, */
    /* 1 for dots/inch, or 2 for dots/cm.  Note that the pixel aspect */
    /* ratio is defined by X_density/Y_density even when density_unit=0. */
    switch (cinfo->density_unit)           /* JFIF code for pixel size units */
      {
      case 1: image.xres = cinfo->X_density;		/* Horizontal pixel density */
	image.yres = cinfo->Y_density;		/* Vertical pixel density */
	break;
      case 2: image.xres = cinfo->X_density * 254 / 100;
	image.yres = cinfo->Y_density * 254 / 100;
	break;
      default:
	image.xres = image.yres = 0;
      }
  }
  
  /* Step 8: Release JPEG decompression object */
  
  /* This is an important step since it will release a good deal of memory. */
  jpeg_finish_decompress(cinfo);
  jpeg_destroy_decompress (cinfo);
}

bool JPEGCodec::do_transform (JXFORM_CODE code, Image& image, bool to_gray)
{
  jpeg_transform_info transformoption; /* image transformation options */
  
  jpeg_decompress_struct srcinfo;
  jpeg_compress_struct dstinfo;
  jpeg_error_mgr jsrcerr, jdsterr;
  
  std::stringstream stream;
  
#ifdef PROGRESS_REPORT
  struct cdjpeg_progress_mgr progress;
#endif
  
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

  cpp_stream_src (&srcinfo, &private_copy);
  
  /* Read file header */
  jpeg_read_header(&srcinfo, TRUE);
  
  transformoption.transform = code;
  transformoption.trim = TRUE;
  transformoption.force_grayscale = to_gray ? TRUE : FALSE;
  
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
  dst_coef_arrays = jtransform_adjust_parameters(&srcinfo, &dstinfo,
						 src_coef_arrays,
						 &transformoption);

  /* Specify data destination for compression */
  cpp_stream_dest (&dstinfo, &stream);
  
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
  
  // copy into the shadow buffer
  private_copy.str (stream.str());
  
  // Update meta, w/h,spp might have changed
  // we re-read the header because we do not want to re-hardcode the
  // trimming required for some operations
  readMeta (&private_copy, image, true /* just basic, e.g. no res, etc. */);
  // as we read back just the basics, some manual translations
  switch (code) {
  case JXFORM_ROT_90:
  case JXFORM_ROT_270:
    { int t = image.xres; image.xres = image.yres; image.yres = t;} break;
  default:
    ; // silence compiler
  }
  
  return true;
}

JPEGCodec jpeg_loader;
