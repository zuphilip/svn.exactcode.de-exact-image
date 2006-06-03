
#include "ImageLoader.hh"

class DCRAWLoader : public ImageLoader {
public:
  
  DCRAWLoader () { registerLoader ("cr2", this); };
  virtual ~DCRAWLoader () { unregisterLoader (this); };

  virtual bool readImage (FILE* file, Image& image);
  virtual bool writeImage (FILE* file, Image& image, int quality, const std::string& compress);
};
