#include "Codecs.hh"

class JPEGCodec : public ImageCodec {
public:
  
  JPEGCodec () {
    registerCodec ("jpeg", this);
    registerCodec ("jpg", this);
  };
  
  virtual std::string getID () { return "JPEG"; };
  
  virtual bool readImage (std::istream* stream, Image& image);
  virtual bool writeImage (std::ostream* stream, Image& image,
			   int quality, const std::string& compress);
  
  // optional optimizing and/or lossless implementations
  virtual bool flipX (Image& image);
  virtual bool flipY (Image& image);
  virtual bool rotate (Image& image, double angle);
  
  virtual bool scale (Image& image, double xscale, double yscale);
};
