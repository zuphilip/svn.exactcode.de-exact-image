
#include "Codecs.hh"

// not TIFFCodec due the TIFFCodec in libtiff itself ...
class TIFCodec : public ImageCodec {
public:
  
  TIFCodec () {
    registerCodec ("tiff", this);
    registerCodec ("tif", this);
  };

  ~TIFCodec () { unregisterCodec (this); };
  
  virtual bool readImage (FILE* file, Image& image);
  virtual bool writeImage (FILE* file, Image& image, int quality, const std::string& compress);
};
