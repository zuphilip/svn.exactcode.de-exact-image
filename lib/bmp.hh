
#include "ImageLoader.hh"

class BMPLoader : public ImageLoader {
public:
  
  BMPLoader () { registerLoader ("bmp", this); };
  ~BMPLoader () { unregisterLoader (this); };

  virtual bool readImage (const char* file, Image& image);
  virtual bool writeImage (const char* file, Image& image);
};
