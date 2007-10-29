extern "C" {
#include <jpeglib.h>
#include <jerror.h>
#include "transupp.h"
}

#include <sstream>

#include "Codecs.hh"

class JPEGCodec : public ImageCodec {
public:
  
  JPEGCodec () {
    registerCodec ("jpeg", this);
    registerCodec ("jpg", this);
  };
  
  // freestanding
  JPEGCodec (Image* _image);
  
  virtual std::string getID () { return "JPEG"; };
  
  virtual bool readImage (std::istream* stream, Image& image);
  virtual bool writeImage (std::ostream* stream, Image& image,
			   int quality, const std::string& compress);
  
  // on-demand decoding
  virtual /*bool*/ void decodeNow (Image* image);
  
  // optional optimizing and/or lossless implementations
  virtual bool flipX (Image& image);
  virtual bool flipY (Image& image);
  virtual bool rotate (Image& image, double angle);
  virtual bool crop (Image& image, unsigned int x, unsigned int y, unsigned int w, unsigned int h);
  virtual bool toGray (Image& image);
  virtual bool scale (Image& image, double xscale, double yscale);
  
private:

  void decodeNow (Image* image, int factor);
  
  // internals and helper
  bool readMeta (std::istream* stream, Image& image);
  bool doTransform (JXFORM_CODE code, Image& image,
		    std::ostream* stream = 0, bool to_gray = false, bool crop = false,
		    unsigned int x = 0, unsigned int y = 0, unsigned int w = 0, unsigned int h = 0);
  
  std::stringstream private_copy;
};
