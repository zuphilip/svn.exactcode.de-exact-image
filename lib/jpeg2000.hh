
#include "ImageLoader.hh"

class JPEG2000Loader : public ImageLoader {
public:
  
  JPEG2000Loader () { registerLoader ("jp2", this); };
  virtual ~JPEG2000Loader () { unregisterLoader (this); };

  virtual 
  unsigned char*
  readImage (const char* filename, int* w, int* h, int* bps, int* spp,
	     int* xres, int* yres);
  
  virtual void
  writeImage (const char* file, unsigned char* data, int w, int h, int bps, int spp,
	      int xres, int yres);
};
