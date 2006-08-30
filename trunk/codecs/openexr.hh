
#include "Codecs.hh"

class OpenEXRLoader : public ImageLoader {
public:
  
  OpenEXRLoader () {
    registerLoader ("exr", this);
  };

  ~OpenEXRLoader () { unregisterLoader (this); };
  
  virtual bool readImage (FILE* file, Image& image);
  virtual bool writeImage (FILE* file, Image& image, int quality, const std::string& compress);
};
