
#include "Codecs.hh"

class XPMCodec : public ImageCodec {
public:
  
  XPMCodec () {
    registerCodec ("xpm", this);
  };
  
  virtual std::string getID () { return "XPM"; };
  
  virtual bool readImage (std::istream* stream, Image& image);
  virtual bool writeImage (std::ostream* stream, Image& image,
			   int quality, const std::string& compress);
};
