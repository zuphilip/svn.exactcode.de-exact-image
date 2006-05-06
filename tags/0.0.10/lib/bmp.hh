
#include "ImageLoader.hh"

class BMPLoader : public ImageLoader {
public:
  
  BMPLoader () { registerLoader ("bmp", this); };
  ~BMPLoader () { unregisterLoader (this); };

  virtual bool readImage (FILE* file, Image& image);
  virtual bool writeImage (FILE* file, Image& image);
};
