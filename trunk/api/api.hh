
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

class Image; // just forward, never ever care about the internal layout

// instanciate new image class
Image* newImage ();

// destroy image instance
void deleteImage (Image* image);


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
  printf ("here in SWIG: %p, %d\n", *$1, *$2);
  
  $target = sv_newmortal();
  if (*$1 && *$2) {
    sv_setpvn ($target, *$1, *$2);
  } else {
    sv_setsv($target, &PL_sv_undef);
  }
  argvi++;
  XSRETURN(argvi);
};
#endif
#endif
  void encodeImage (char **s, int *slen,
		    Image* image, const char* codec, int quality,
		    const char* compression);

// encode image into specified filename
bool encodeImageFile (Image* image, const char* filename,
		      int quality, const char* compression);


// image properties
int imageChannels (Image* image);
int imageChannelDepth (Image* image);

int imageWidth (Image* image);
int imageHeight (Image* image);

int imageXres (Image* image);
int imageYres (Image* image);

// image manipulation
void imageRotate (Image* image, double angle);
