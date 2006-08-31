
#include "Codecs.hh"

class PNGCodec : public ImageCodec {
public:
  
  PNGCodec () { registerCodec ("png", this); };
  virtual ~PNGCodec () { unregisterCodec (this); };

  virtual bool readImage (std::istream* stream, Image& image);
  virtual bool writeImage (std::ostream* stream, Image& image, int quality, const std::string& compress);
};
