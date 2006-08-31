
#include "Codecs.hh"

class OpenEXRCodec : public ImageCodec {
public:
  
  OpenEXRCodec () {
    registerCodec ("exr", this);
  };

  ~OpenEXRCodec () { unregisterCodec (this); };
  
  virtual bool readImage (std::istream* stream, Image& image);
  virtual bool writeImage (std::ostream* stream, Image& image, int quality,
			   const std::string& compress);
};
