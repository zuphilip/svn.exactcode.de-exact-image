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
  
  virtual bool scale (Image& image, double xscale, double yscale);
  
private:
  
  // internals and helper
  bool readMeta (std::istream* stream, Image& image);
  bool doTransform (JXFORM_CODE code, Image& image, bool to_gray = false);
  
  std::stringstream private_copy;
};
