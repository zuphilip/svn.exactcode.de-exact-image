
#include "ImageLoader.hh"

class PNGLoader : public ImageLoader {
public:
  
  PNGLoader () { registerLoader ("png", this); };
  virtual ~PNGLoader () { unregisterLoader (this); };

  virtual bool readImage (const char* file, Image& image);
  virtual bool writeImage (const char* file, Image& image);
};
