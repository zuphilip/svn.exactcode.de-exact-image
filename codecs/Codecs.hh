#ifndef IMAGELOADER_HH
#define IMAGELOADER_HH

#include <stdio.h>

#include <vector>
#include <algorithm>
#include <iosfwd>

#include "Image.hh"

/* The Image decoder and coder collection.
 *
 * The class itself has static methods to perform the de- and
 * encoding. These methods search thru a loader vector to match the
 * file magick (on decoding) or the specified codec / file extension
 * on encoding.
 *
 * The codec might attach itself to the image on decoding to allow
 * on-the-fly decoding and reuse of coded data while re-encoding an
 * image where nothing or just meta data was touched while the pixel
 * data remain unmodified.
 */

// just forward
class Image;

class ImageCodec
{
public:
  
  virtual ~ImageCodec ();
  virtual std::string getID () = 0;
  
  // NEW API, allowing the use of any STL i/o stream derived source.
  static bool Read (std::istream* stream, Image& image,
		    std::string codec = "");
  static bool Write (std::ostream* stream, Image& image,
		     std::string codec = "", std::string ext = "",
		     int quality = 75, const std::string& compress = "");
  
  // OLD API, only left for compatibility.
  // Not const string& because the filename is parsed and the copy is changed intern.
  // 
  // Removed after 2007-06-01!
  static bool Read (std::string file, Image& image);
  static bool Write (std::string file, Image& image,
		     int quality = 75, const std::string& compress = "");
  
  // per codec methods
  virtual bool readImage (std::istream* stream, Image& image) = 0;
  virtual bool writeImage (std::ostream* stream, Image& image,
			   int quality, const std::string& compress) = 0;
  
  // not pure-virtual so not every codec needs a NOP
  virtual /*bool*/ void decodeNow (Image* image);
  
  // optional optimizing and/or lossless implementations (JPEG, et.al.)
  virtual bool flipX (Image* image);
  virtual bool flipY (Image* image);
  virtual bool rotate (Image& image, double angle);
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
  
};

#endif
