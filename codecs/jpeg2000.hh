
#include "Codecs.hh"

class JPEG2000Codec : public ImageCodec {
public:
  
  JPEG2000Codec () { registerCodec ("jp2", this); };
  virtual ~JPEG2000Codec () { unregisterCodec (this); };

  virtual bool readImage (FILE* file, Image& im);
  virtual bool writeImage (FILE* file, Image& im, int quality, const std::string& compress);
};
