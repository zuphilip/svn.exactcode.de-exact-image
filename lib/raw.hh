#include "ImageLoader.hh"

class RAWLoader : public ImageLoader {
public:
  
  RAWLoader () { registerLoader ("raw", this); };
  virtual ~RAWLoader () { unregisterLoader (this); };

  virtual bool readImage (FILE* filename, Image& image);
  virtual bool writeImage (FILE* file, Image& image, int quality);
};
