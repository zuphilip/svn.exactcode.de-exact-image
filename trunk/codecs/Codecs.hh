/*
 * Copyright (C) 2006 - 2015 René Rebe
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
 * encoding. These methods search thru a loader list to match the
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

#include <list>
#include <set>

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

  // helpers for parsing the codec spec and filename extension
  static std::string getCodec (std::string& filename);
  static std::string getExtension (const std::string& filename);

  // NEW API, allowing the use of any STL i/o stream derived source. The index i
  // is the page, image, index number within the file (TIFF, GIF, ICO, etc.)
  static int Read (std::istream* stream, Image& image,
		   std::string codec = "", const std::string& decompress = "", int index = 0);
  static bool Write (std::ostream* stream, Image& image,
		     std::string codec, std::string ext = "",
		     int quality = 75, const std::string& compress = "");

  static ImageCodec* MultiWrite (std::ostream* stream,
				 std::string codec, std::string ext = "");
  
  // OLD API, only left for compatibility.
  // Not const string& because the filename is parsed and the copy is changed intern.
  // 
  // Removed soon!
  static int Read (std::string file, Image& image, const std::string& decompress = "", int index = 0);
  static bool Write (std::string file, Image& image,
		     int quality = 75, const std::string& compress = "");
  
  // Per codec methods, only one set needs to be implemented, the one with
  // index invoke the one without by default (compatibility, ease of implemntation
  // fallback), the ones without return error (as in "not implemented").
  virtual int readImage (std::istream* stream, Image& image,
			 const std::string& decompress);
  virtual int readImage (std::istream* stream, Image& image,
			 const std::string& decompress, int index);

  virtual bool writeImage (std::ostream* stream, Image& image,
			   int quality, const std::string& compress) = 0;
  virtual ImageCodec* instanciateForWrite (std::ostream* stream);
  // slightly named differently to match the public factory name
  virtual bool Write (Image& image,
		      int quality = 75, const std::string& compress = "", int index = 0);
  
  // not pure-virtual so not every codec needs a NOP
  virtual /*bool*/ void decodeNow (Image* image);
  
  // optional optimizing and/or lossless implementations (JPEG, et al.)
  virtual bool flipX (Image& image);
  virtual bool flipY (Image& image);
  virtual bool rotate (Image& image, double angle);
  virtual bool crop (Image& image, unsigned int x, unsigned int y, unsigned int w, unsigned int h);
  virtual bool toGray (Image& image);
  virtual bool scale (Image& image, double xscale, double yscale, bool fixed);
  
protected:
  
  struct loader_ref {
    const char* ext;
    ImageCodec* loader;
    bool primary_entry;
    bool via_codec_only;
  };
  
  static std::list<loader_ref>* loader;
  
  static void registerCodec (const char* _ext,
			     ImageCodec* _loader,
			     bool _via_codec_only = false,
			     bool push_back = false);
  static void unregisterCodec (ImageCodec* _loader);
  
  // freestanding instance, attached to an image
  const Image* _image;
};

class Args
{
public:
  Args(const std::string& _args, bool ignoreCase = true)
  {
    for (size_t it1 = 0; it1 < _args.size();) {
      size_t it2 = _args.find_first_of(",;|", it1);
      
      if (it2 == std::string::npos)
	it2 = _args.size();
      
      std::string s = _args.substr(it1, it2 - it1);
      if (ignoreCase)
	std::transform(s.begin(), s.end(), s.begin(), tolower);
      args.insert(s);
      
      it1 = it2 + 1;
    }
  }
  
  bool contains(const std::string& arg)
  {
    if (args.find(arg) != args.end())
      return true;
    else
      return false;
  }
  
  void remove(const std::string& arg)
  {
    args.erase(arg);
  }
  
  bool containsAndRemove(const std::string& arg)
  {
    if (contains(arg)) {
      remove(arg);
      return true;
    }
    return false;
  }
  
  std::string str()
  {
    std::string ret;
    std::set<std::string>::iterator it = args.begin();
    if (it != args.end())
      ret = *it++;
    
    for (; it != args.end(); ++it) {
      ret += ",";
      ret += *it;
    }
    return ret;
  }
  
protected:
  std::set<std::string> args;
};


#endif
