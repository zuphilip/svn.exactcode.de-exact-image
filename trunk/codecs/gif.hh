
#include "Codecs.hh"

class GIFCodec : public ImageCodec {
public:
  
  GIFCodec () { registerCodec ("gif", this); };
  virtual ~GIFCodec () { unregisterCodec (this); };

  virtual bool readImage (FILE* file, Image& image);
  virtual bool writeImage (FILE* file, Image& image, int quality, const std::string& compress);
};
