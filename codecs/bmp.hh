
#include "Codecs.hh"

class BMPCodec : public ImageCodec {
public:
  
  BMPCodec () { registerCodec ("bmp", this); };
  
  virtual std::string getID () { return "BMP"; };

  virtual bool readImage (std::istream* stream, Image& image);
  virtual bool writeImage (std::ostream* stream, Image& image, int quality, const std::string& compress);
};
