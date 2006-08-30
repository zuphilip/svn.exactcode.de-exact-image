#include "Codecs.hh"

class RAWCodec : public ImageCodec {
public:
  
  RAWCodec () { registerCodec ("raw", this, true /* explicit only */); };
  virtual ~RAWCodec () { unregisterCodec (this); };

  virtual bool readImage (FILE* filename, Image& image);
  virtual bool writeImage (FILE* file, Image& image, int quality, const std::string& compress);
};
