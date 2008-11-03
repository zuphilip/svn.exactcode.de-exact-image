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

/* The Image decoder and coder collection.
 *
 * The class itself has static methods to perform the de- and
 * encoding. These methods search thru a loader vector to match the
 * file magick (on decoding) or the specified codec / file extension
 * on encoding.
 *
 * The codec might attach a freestanding instance of itself onto the
 * image on decode to allow on-the-fly decoding and reuse of coded
 * data while re-encoding an image where nothing or just meta data was
 * touched while the pixel data remains unmodified.
 *
 * Also some specialized methods are available to optimize some
 * operations that sometimes can work on encoded data.
 */

#ifndef IMAGELOADER_HH
#define IMAGELOADER_HH

#include <stdio.h>

#include <vector>
#include <algorithm>
#include <iosfwd>

#include "Image.hh"

// just forward
class Image;

class ImageCodec
{
public:
  ImageCodec ();
  ImageCodec (Image* __image);
  virtual ~ImageCodec ();
  virtual std::string getID () = 0;
  
  // NEW API, allowing the use of any STL i/o stream derived source.
  static bool Read (std::istream* stream, Image& image,
		    std::string codec = "", const std::string& decompress = "");
  static bool Write (std::ostream* stream, Image& image,
		     std::string codec, std::string ext = "",
		     int quality = 75, const std::string& compress = "");
  
  // OLD API, only left for compatibility.
  // Not const string& because the filename is parsed and the copy is changed intern.
  // 
  // Removed after 2000-01-01!
  static bool Read (std::string file, Image& image, const std::string& decompress = "");
  static bool Write (std::string file, Image& image,
		     int quality = 75, const std::string& compress = "");
  
  // per codec methods
  virtual bool readImage (std::istream* stream, Image& image,
			  const std::string& decompress = "") = 0;
  virtual bool writeImage (std::ostream* stream, Image& image,
			   int quality, const std::string& compress) = 0;
  
  // not pure-virtual so not every codec needs a NOP
  virtual /*bool*/ void decodeNow (Image* image);
  
  // optional optimizing and/or lossless implementations (JPEG, et al.)
  virtual bool flipX (Image& image);
  virtual bool flipY (Image& image);
  virtual bool rotate (Image& image, double angle);
  virtual bool crop (Image& image, unsigned int x, unsigned int y, unsigned int w, unsigned int h);
  virtual bool toGray (Image& image);
  virtual bool scale (Image& image, double xscale, double yscale);
  
protected:
  
  struct loader_ref {
    const char* ext;
    ImageCodec* loader;
    bool primary_entry;
    bool via_codec_only;
  };
  
  static std::vector<loader_ref>* loader;
  
  static void registerCodec (const char* _ext,
			     ImageCodec* _loader,
			     bool _via_codec_only = false);
  static void unregisterCodec (ImageCodec* _loader);
  
  // freestanding instance, attached to an image
  const Image* _image;
};

#endif
