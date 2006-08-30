#include "Codecs.hh"

class JPEGCodec : public ImageCodec {
public:
  
  JPEGCodec () {
    registerCodec ("jpeg", this);
    registerCodec ("jpg", this);
  };
  virtual ~JPEGCodec () { unregisterCodec (this); };

  virtual bool readImage (FILE* filename, Image& image);
  virtual bool writeImage (FILE* file, Image& image, int quality, const std::string& compress);
};
