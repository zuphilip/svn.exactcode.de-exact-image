#include "Codecs.hh"

extern "C" {
#include <jpeglib.h>
#include <jerror.h>
}

class JPEGCodec : public ImageCodec {
public:
  
  JPEGCodec ()
    : srcinfo (0), src_coef_arrays (0)
  {
    registerCodec ("jpeg", this);
    registerCodec ("jpg", this);
  };
  
  ~JPEGCodec ();
  
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
  // for on-demand loading
  jpeg_decompress_struct* srcinfo;
  // for encoding reuse
  jvirt_barray_ptr* src_coef_arrays;

};
