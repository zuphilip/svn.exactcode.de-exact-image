
#include "Codecs.hh"

class PNMCodec : public ImageCodec {
public:
  
  PNMCodec () {
    registerCodec ("pnm", this);
    registerCodec ("ppm", this);
    registerCodec ("pgm", this);
    registerCodec ("pbm", this);
  };
  virtual ~PNMCodec () { unregisterCodec (this); };

  virtual bool readImage (FILE* file, Image& image);
  virtual bool writeImage (FILE* file, Image& image, int quality, const std::string& compress);
};
