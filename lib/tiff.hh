
#include "ImageLoader.hh"

class TIFFLoader : public ImageLoader {
public:
  
  TIFFLoader () {
    registerLoader ("tif", this);
    registerLoader ("tiff", this);
  };

  ~TIFFLoader () { unregisterLoader (this); };
  
  virtual bool readImage (FILE* file, Image& image);
  virtual bool writeImage (FILE* file, Image& image, int quality);
};
