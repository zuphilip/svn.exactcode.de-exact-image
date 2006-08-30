
#include "Codecs.hh"

class DCRAWCodec : public ImageCodec {
public:
  
  DCRAWCodec () {
    registerCodec ("dcraw", this); // primary entry
    registerCodec ("crw", this); // Canon
    registerCodec ("cr2", this);
    registerCodec ("mrw", this); // Mniolta
    registerCodec ("nef", this); // Nikon
    registerCodec ("orf", this); // Olympus
    registerCodec ("raf", this); // Fuji
    registerCodec ("pef", this); // Pentax
    registerCodec ("x3f", this); // Sigma
    registerCodec ("dcr", this); // Kodak
    registerCodec ("kdc", this); 
    registerCodec ("srf", this); // Sony
    registerCodec ("raw", this); // Panasonic, Casio, Leica
  };
  virtual ~DCRAWCodec () { unregisterCodec (this); };

  virtual bool readImage (FILE* file, Image& image);
  virtual bool writeImage (FILE* file, Image& image, int quality, const std::string& compress);
};
