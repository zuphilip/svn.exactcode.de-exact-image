
#include "Codecs.hh"

class PNGCodec : public ImageCodec {
public:
  
  PNGCodec () { registerCodec ("png", this); };
  virtual ~PNGCodec () { unregisterCodec (this); };

  virtual bool readImage (FILE* file, Image& image);
  virtual bool writeImage (FILE* file, Image& image, int quality, const std::string& compress);
};
