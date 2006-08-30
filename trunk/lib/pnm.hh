
#include "Codecs.hh"

class PNMLoader : public ImageLoader {
public:
  
  PNMLoader () {
    registerLoader ("pnm", this);
    registerLoader ("ppm", this);
    registerLoader ("pgm", this);
    registerLoader ("pbm", this);
  };
  virtual ~PNMLoader () { unregisterLoader (this); };

  virtual bool readImage (FILE* file, Image& image);
  virtual bool writeImage (FILE* file, Image& image, int quality, const std::string& compress);
};
