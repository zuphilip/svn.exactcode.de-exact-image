
#include "Codecs.hh"

class GIFLoader : public ImageLoader {
public:
  
  GIFLoader () { registerLoader ("gif", this); };
  virtual ~GIFLoader () { unregisterLoader (this); };

  virtual bool readImage (FILE* file, Image& image);
  virtual bool writeImage (FILE* file, Image& image, int quality, const std::string& compress);
};
