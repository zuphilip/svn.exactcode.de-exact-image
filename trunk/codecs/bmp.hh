
#include "Codecs.hh"

class BMPCodec : public ImageCodec {
public:
  
  BMPCodec () { registerCodec ("bmp", this); };
  ~BMPCodec () { unregisterCodec (this); };

  virtual bool readImage (FILE* file, Image& image);
  virtual bool writeImage (FILE* file, Image& image, int quality, const std::string& compress);
};
