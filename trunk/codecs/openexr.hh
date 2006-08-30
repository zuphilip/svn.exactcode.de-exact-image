
#include "Codecs.hh"

class OpenEXRCodec : public ImageCodec {
public:
  
  OpenEXRCodec () {
    registerCodec ("exr", this);
  };

  ~OpenEXRCodec () { unregisterCodec (this); };
  
  virtual bool readImage (FILE* file, Image& image);
  virtual bool writeImage (FILE* file, Image& image, int quality, const std::string& compress);
};
