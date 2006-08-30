#include "Codecs.hh"

class RAWLoader : public ImageLoader {
public:
  
  RAWLoader () { registerLoader ("raw", this, true /* explicit only */); };
  virtual ~RAWLoader () { unregisterLoader (this); };

  virtual bool readImage (FILE* filename, Image& image);
  virtual bool writeImage (FILE* file, Image& image, int quality, const std::string& compress);
};
