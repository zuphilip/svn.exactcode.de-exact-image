
#include "Codecs.hh"

class JPEG2000Codec : public ImageCodec {
public:
  
  JPEG2000Codec () { registerCodec ("jp2", this); };
  virtual ~JPEG2000Codec () { unregisterCodec (this); };

  virtual bool readImage (std::istream* stream, Image& im);
  virtual bool writeImage (std::ostream* stream, Image& im, int quality, const std::string& compress);
};
