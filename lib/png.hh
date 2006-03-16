
#include "ImageLoader.hh"

class PNGLoader : public ImageLoader {
public:
  
  PNGLoader () { registerLoader ("png", this); };

  virtual 
  unsigned char*
  readImage (const char* file, int* w, int* h, int* bps, int* spp,
	     int* xres, int* yres);

  virtual void
  writeImage (const char* file, unsigned char* data, int w, int h, int bps, int spp,
	      int xres, int yres);
};
