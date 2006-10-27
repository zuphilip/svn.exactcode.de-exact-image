/*
 * The ExactImage stable external API for use with SWIG.
 * Copyright (C) 2006 René Rebe, Archivista
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

/*
 * This header defines the public, supposedly stable public API that
 * can even be used from C as well as SWIG script language bindings.
 *
 * We decided to map the library internals in an non-OO (Object
 * Oriented) way in order to archive the most flexible external
 * language choice possible and not to hold of refactoring from
 * time to time.
 *
 * (People demanding a detailed and Object Oriented interface
 *  can still choose to use the fine grained intern C++ API
 *  [which however is not set in stone {at least not yet}].)
 */

#include "config.h"

class Image; // just forward, never ever care about the internal layout

// instanciate new image class
Image* newImage ();

// destroy image instance
void deleteImage (Image* image);

// copy the image's pixel and meta data
Image* copyImage (Image* image);


// decode image from memory data of size n
#ifdef SWIG
%apply (char *STRING, int LENGTH) { (char *data, int n) };
#endif
bool decodeImage (Image* image, char* data, int n);

// decode image from given filename
bool decodeImageFile (Image* image, const char* filename);


// encode image to memory, the data is newly allocated and returned
// return 0 i the image could not be decoded

#ifdef SWIG
#if 0
%cstring_output_allocate_size( char ** s, int *slen, free(*$1))
#else
 // THIS WORKS AROUND WHAT I BELIVE IS A SWIG BUG !!!
 // %typemap(perl5,argout) (char **s, len *slen)  {
 // if (*$1) {
 //   $result = sv_newmortal();
 //   sv_setpvn ($result, (*$1)->data, (*$1)->len);
 // }
 // else
 //   $result = &PL_sv_undef;
 // argvi++;
%typemap(in, numinputs=0) (char** s, int* slen) (char* a, int b) {
  $1 = &a;
  $2 = &b;
}
%typemap(argout) (char** s, int* slen) {
  $result = sv_newmortal();
  if (*$1 && *$2) {
    sv_setpvn ($result, *$1, *$2);
  } else {
    sv_setsv($result, &PL_sv_undef);
  }
  argvi++;
  XSRETURN(argvi);
};
#endif
#endif
  void encodeImage (char **s, int *slen,
		    Image* image, const char* codec, int quality = 100,
		    const char* compression = "");

// encode image into specified filename
bool encodeImageFile (Image* image, const char* filename,
		      int quality = 100, const char* compression = "");


// image properties
int imageChannels (Image* image);
int imageChannelDepth (Image* image);

int imageWidth (Image* image);
int imageHeight (Image* image);

// returns the name of the image colorspace such as gray, gray2, gray4, rgb8, rgb16, cymk8, cymk16 ...
char* imageColorspace (Image* image);

// returns X and Y resolution
int imageXres (Image* image);
int imageYres (Image* image);

// set X and Y resolution
void imageSetXres (Image* image, int xres);
void imageSetYres (Image* image, int yres);


// image manipulation

// transforms the image into a new target colorspace - the names are the same as returned by
// imageColorspace, might return false if the conversion was not possible
bool imageConvertColorspace (Image* image, const char* target_colorspace);
void imageRotate (Image* image, double angle);
void imageScale (Image* image, double factor);
void imageBoxScale (Image* image, double factor);

// advanced all-in-one algorithms
void imageOptimize2BW (Image* image, int low = 0, int high = 255,
		       int threshold = 170,
		       int radius = 3, double standard_deviation = 2.3,
		       int target_dpi = 0);
// remeber the margin must be a multiple of 8!
bool imageIsEmpty (Image* image, double percent, int margin);

#if WITHBARDECODE == 1
// commercial bardecode
// codes is the string of barcode to look for, | seperated, like:
// CODE39|CODE128|CODE25|EAN13|EAN8|UPCA|UPCE
// case doesn't matter
// 
// returned is an alternating array of codes and types, like
// "1234", "EAN", "5678", "CODE128", ...
#ifdef SWIG
// Creates a new Perl array and places a NULL-terminated char ** into it
%typemap(out) char** {
  AV *myav;
  SV **svs;
  int i = 0,len = 0;
  /* Figure out how many elements we have */
  while ($1[len])
    len++;
  svs = (SV **) malloc(len*sizeof(SV *));
  for (i = 0; i < len ; i++) {
    svs[i] = sv_newmortal();
    sv_setpv((SV*)svs[i],$1[i]);
    free ($1[i]);
  };
  myav =  av_make(len,svs);
  free(svs);
  free($1);
  $result = newRV((SV*)myav);
  sv_2mortal($result);
  argvi++;
}
#endif
char** imageDecodeBarcodes (Image* image, const char* codes,
			    int min_length, int max_length);
#endif
