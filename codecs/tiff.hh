
#include "Codecs.hh"

// not TIFFCodec due the TIFFCodec in libtiff itself ...
class TIFCodec : public ImageCodec {
public:
  
  TIFCodec () {
    registerCodec ("tiff", this);
    registerCodec ("tif", this);
  };

  ~TIFCodec () { unregisterCodec (this); };
  
  virtual bool readImage (std::istream* stream, Image& image);
  virtual bool writeImage (std::ostream* stream, Image& image, int quality, const std::string& compress);
};
