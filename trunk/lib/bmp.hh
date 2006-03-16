
#include "ImageLoader.hh"

class BMPLoader : public ImageLoader {
public:
  
  BMPLoader () { registerLoader ("bmp", this); };
  ~BMPLoader () { unregisterLoader (this); };

  virtual unsigned char*
  readImage (const char* file, int* w, int* h, int* bps, int* spp,
	     int* xres, int* yres);
  
  virtual void
  writeImage (const char* file, unsigned char* data, int w, int h,
	      int bps, int spp, int xres, int yres);
  
};
