
#include "Codecs.hh"

class OpenEXRCodec : public ImageCodec {
public:
  
  OpenEXRCodec () { registerCodec ("exr", this); };
  
    virtual std::string getID () { return "OpenEXR"; };

  
  virtual bool readImage (std::istream* stream, Image& image);
  virtual bool writeImage (std::ostream* stream, Image& image, int quality,
			   const std::string& compress);
};
