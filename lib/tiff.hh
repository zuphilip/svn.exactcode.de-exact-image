
#include "ImageLoader.hh"

class TIFFLoader : public ImageLoader {
public:
  
  TIFFLoader () {
    registerLoader ("tif", this);
    registerLoader ("tiff", this);
  };

  ~TIFFLoader () { unregisterLoader (this); };
  
  virtual bool readImage (const char* file, Image& image);
  virtual bool writeImage (const char* file, Image& image);
};
