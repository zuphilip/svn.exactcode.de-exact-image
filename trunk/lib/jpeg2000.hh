
#include "ImageLoader.hh"

class JPEG2000Loader : public ImageLoader {
public:
  
  JPEG2000Loader () { registerLoader ("jp2", this); };
  virtual ~JPEG2000Loader () { unregisterLoader (this); };

  virtual bool readImage (const char* filename, Image& im);
  virtual bool writeImage (const char* filename, Image& im);
};
