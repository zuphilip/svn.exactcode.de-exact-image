
#include "ImageLoader.hh"

class DCRAWLoader : public ImageLoader {
public:
  
  DCRAWLoader () {
    registerLoader ("crw", this); // Canon
    registerLoader ("cr2", this);
    registerLoader ("mrw", this); // Mniolta
    registerLoader ("nef", this); // Nikon
    registerLoader ("orf", this); // Olympus
    registerLoader ("raf", this); // Fuji
    registerLoader ("pef", this); // Pentax
    registerLoader ("x3f", this); // Sigma
    registerLoader ("dcr", this); // Kodak
    registerLoader ("kdc", this); 
    registerLoader ("srf", this); // Sony
    // registerLoader ("raw", this); // Panasonic, Casio, Leica, conflict with real "raw" handler :-(
  };
  virtual ~DCRAWLoader () { unregisterLoader (this); };

  virtual bool readImage (FILE* file, Image& image);
  virtual bool writeImage (FILE* file, Image& image, int quality, const std::string& compress);
};
