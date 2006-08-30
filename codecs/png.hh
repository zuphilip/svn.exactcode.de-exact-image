
#include "Codecs.hh"

class PNGLoader : public ImageLoader {
public:
  
  PNGLoader () { registerLoader ("png", this); };
  virtual ~PNGLoader () { unregisterLoader (this); };

  virtual bool readImage (FILE* file, Image& image);
  virtual bool writeImage (FILE* file, Image& image, int quality, const std::string& compress);
};
