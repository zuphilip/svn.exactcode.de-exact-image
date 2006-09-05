
#include "Codecs.hh"

class GIFCodec : public ImageCodec {
public:
  
  GIFCodec () { registerCodec ("gif", this); };
  virtual ~GIFCodec () { unregisterCodec (this); };

  virtual bool readImage (std::istream* stream, Image& image);
  virtual bool writeImage (std::ostream* stream, Image& image, int quality, const std::string& compress);
};
