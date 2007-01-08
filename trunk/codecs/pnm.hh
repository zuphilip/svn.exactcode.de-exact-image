
#include "Codecs.hh"

class PNMCodec : public ImageCodec {
public:
  
  PNMCodec () {
    registerCodec ("pnm", this);
    registerCodec ("ppm", this);
    registerCodec ("pgm", this);
    registerCodec ("pbm", this);
  };
  
  virtual std::string getID () { return "PNM"; };
  
  virtual bool readImage (std::istream* stream, Image& image);
  virtual bool writeImage (std::ostream* stream, Image& image,
			   int quality, const std::string& compress);
};
