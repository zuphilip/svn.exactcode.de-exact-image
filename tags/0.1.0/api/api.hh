
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
 * Oriented) way in order to archive most flexible external
 * language choice possible.
 *
 * (People demanding a detailed and Object Oriented interface
 *  can still choose to use the fine grained intern C++ API
 *  [which however is not set in stone yet].)
 */

#ifdef SWIG
%module exactimage
%{
#endif

#include <iostream>

class Image; // just forward

// from / to memory
bool decodeImage (Image& image, const char* filename);
bool ecodeImage (Image& image, const char* filename);

// from / to file
bool decodeImageFile (Image& image, const char* filename);
bool ecodeImageFile (Image& image, const char* filename);

// TODO: splitImage
//       colorspaceImage
//       normalizeImage
//       scaleImage
//       scaleNearestImage
//       scaleBilinearImage
//       scaleBicubicImage
//       scaleBoxImage
//       scaleDDTImage
//       shearImage

//       flipImage
//       flopImage
//       rotateImage

//       ditherFloydSteinbergImage
//       ditherRiemersmaImage

//       edgeImage 
//       descewImage

#ifdef SWIG
 %}
#endif
