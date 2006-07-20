
#include "ImageLoader.hh"

class TIFFLoader : public ImageLoader {
public:
  
  TIFFLoader () {
    registerLoader ("tiff", this);
    registerLoader ("tif", this);
  };

  ~TIFFLoader () { unregisterLoader (this); };
  
  virtual bool readImage (FILE* file, Image& image);
  virtual bool writeImage (FILE* file, Image& image, int quality, const std::string& compress);
};
