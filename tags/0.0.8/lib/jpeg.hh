#include "ImageLoader.hh"

class JPEGLoader : public ImageLoader {
public:
  
  JPEGLoader () { registerLoader ("jpg", this); };
  virtual ~JPEGLoader () { unregisterLoader (this); };

  virtual bool readImage (const char* filename, Image& image);
  virtual bool writeImage (const char* file, Image& image);
};
