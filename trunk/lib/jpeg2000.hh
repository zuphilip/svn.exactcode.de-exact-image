
#include "Codecs.hh"

class JPEG2000Loader : public ImageLoader {
public:
  
  JPEG2000Loader () { registerLoader ("jp2", this); };
  virtual ~JPEG2000Loader () { unregisterLoader (this); };

  virtual bool readImage (FILE* file, Image& im);
  virtual bool writeImage (FILE* file, Image& im, int quality, const std::string& compress);
};
