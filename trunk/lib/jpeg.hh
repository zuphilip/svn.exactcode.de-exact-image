#include "ImageLoader.hh"

class JPEGLoader : public ImageLoader {
public:
  
  JPEGLoader () { registerLoader ("jpg", this); };
  virtual ~JPEGLoader () { unregisterLoader (this); };

  virtual bool readImage (FILE* filename, Image& image);
  virtual bool writeImage (FILE* file, Image& image);
};
