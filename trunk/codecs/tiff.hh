
#include "Codecs.hh"

// not TIFFCodec due the TIFFCodec in libtiff itself ...
class TIFCodec : public ImageCodec {
public:
  
  TIFCodec () {
    registerCodec ("tiff", this);
    registerCodec ("tif", this);
  };
  
  virtual std::string getID () { return "TIFF"; };
  
  virtual bool readImage (std::istream* stream, Image& image);
  virtual bool writeImage (std::ostream* stream, Image& image,
			   int quality, const std::string& compress);
  
private:
  
  bool writeImageImpl (TIFF* out, const Image& image, const std::string& conpress, int page = 0);
};
