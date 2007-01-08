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

#include <iostream>

extern "C" {
#include <jpeglib.h>
#include <jerror.h>
}

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
}


void cpp_stream_src (j_decompress_ptr cinfo, std::istream* stream)
{
  cpp_src_mgr* src;

  if (cinfo->src == NULL) {	/* first time for this JPEG object? */
    cinfo->src = (struct jpeg_source_mgr *)
      (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
				  sizeof(cpp_src_mgr));
    src = (cpp_src_mgr*) cinfo->src;
    src->buffer = (JOCTET *)
      (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
				  INPUT_BUF_SIZE * sizeof(JOCTET));
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
}


void cpp_stream_dest (j_compress_ptr cinfo, std::ostream* stream)
{
  cpp_dest_mgr* dest;
  
  /* first time for this JPEG object? */
  if (cinfo->dest == NULL) {
    cinfo->dest = (struct jpeg_destination_mgr *)
      (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
				  sizeof(cpp_dest_mgr));
  }
  
  dest = (cpp_dest_mgr*) cinfo->dest;
  dest->pub.init_destination = init_destination;
  dest->pub.empty_output_buffer = empty_output_buffer;
  dest->pub.term_destination = term_destination;
  dest->stream = stream;
}

/* *** back on-topic *** */

bool JPEGCodec::readImage (std::istream* stream, Image& image)
{
  if (stream->peek () != 0xFF)
    return false;
  stream->get (); // consume silently
  if (stream->peek () != 0xD8)
    return false;
  stream->seekg (0);
  
  if (0) { // TODO: differentiate JFIF vs. Exif?
    // quick magic check
      char buf [10];
      stream->read (buf, sizeof (buf));
      stream->seekg (0);
      
      if (buf[6] != 'J' || buf[7] != 'F' || buf[8] != 'I' || buf[9] != 'F')
	return false;
  }
  
  struct jpeg_decompress_struct* cinfo = new jpeg_decompress_struct;
  
  struct my_error_mgr jerr;
  
  JSAMPROW buffer[1];		/* pointer to JSAMPLE row[s] */
  int row_stride;		/* physical row width in output buffer */
  
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

  (void) jpeg_start_decompress (cinfo);
  
  /* JSAMPLEs per row in output buffer */
  row_stride = cinfo->output_width * cinfo->output_components;

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
    case 1: image.xres = cinfo->X_density;		/* Horizontal pixel density */
            image.yres = cinfo->Y_density;		/* Vertical pixel density */
            break;
    case 2: image.xres = cinfo->X_density * 254 / 100;
            image.yres = cinfo->Y_density * 254 / 100;
            break;
    default:
      image.xres = image.yres = 0;
  }

  image.setRawData ((uint8_t*) malloc(row_stride * cinfo->output_height));
  
  /* Step 6: while (scan lines remain to be read) */
  /*           jpeg_read_scanlines(...); */

  while (! jpeg_input_complete(cinfo)) {
    jpeg_start_output(cinfo, cinfo->input_scan_number);
    while (cinfo->output_scanline < cinfo->output_height) {
      /* jpeg_read_scanlines expects an array of pointers to scanlines.
       * Here the array is only one element long, but you could ask for
       * more than one scanline at a time if that's more convenient.
       */
      buffer[0] = (JSAMPLE*) image.getRawData() + (cinfo->output_scanline*row_stride);
      (void) jpeg_read_scanlines(cinfo, buffer, 1);
    }
    jpeg_finish_output(cinfo);
  }

  /* Step 7: Finish decompression */

#if 1
  jpeg_finish_decompress(cinfo);
  /* We can ignore the return value since suspension is not possible
   * with the stdio data source.
   */

  /* Step 8: Release JPEG decompression object */

  /* This is an important step since it will release a good deal of memory. */
  jpeg_destroy_decompress(cinfo);
  
  /* At this point you may want to check to see whether any corrupt-data
   * warnings occurred (test whether jerr.pub.num_warnings is nonzero).
   */
#else
  if (image.priv_data) {
    jpeg_finish_decompress((jpeg_decompress_struct*)image.priv_data);
    jpeg_destroy_decompress((jpeg_decompress_struct*)image.priv_data);
  }
  image.priv_data = (void*) cinfo; // keep for writing, of course a "HACK"
#endif
  
  return true;
}

bool JPEGCodec::writeImage (std::ostream* stream, Image& image, int quality,
			    const std::string& compress)
{
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
  
#if 0
  if (codec ...) {
    // write original DCT coefficients
    
    std::cerr << "Writing original DCT coefficients." << std::endl;
    
    struct jpeg_decompress_struct* srcinfo = (jpeg_decompress_struct*) image.priv_data;
    
    jpeg_copy_critical_parameters (srcinfo, &cinfo);
    
    cinfo.JFIF_minor_version = 2; // emit JFIF 1.02 extension markers ...
    cinfo.density_unit = 1; /* 1 for dots/inch */
    cinfo.X_density = image.xres;
    cinfo.Y_density = image.yres;
    
    // extract the coefficients
    jvirt_barray_ptr* src_coef_arrays = jpeg_read_coefficients(srcinfo);
    
    jpeg_write_coefficients(&cinfo, src_coef_arrays);
    
    /* Finish compression and release memory */
    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);
    
    jpeg_finish_decompress(srcinfo);
    jpeg_destroy_decompress(srcinfo);
    image.priv_data = 0;
    
    return true;
  }
#endif
  
  // really encode
  
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

JPEGCodec jpeg_loader;
