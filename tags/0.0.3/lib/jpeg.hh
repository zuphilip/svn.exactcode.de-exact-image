#include "ImageLoader.hh"

class JPEGLoader : public ImageLoader {
public:
  
  JPEGLoader () { registerLoader ("jpg", this); };
  virtual ~JPEGLoader () { unregisterLoader (this); };

  virtual 
  unsigned char*
  readImage (const char* filename, int* w, int* h, int* bps, int* spp,
		  int* xres, int* yres);
  
  virtual void
  writeImage (const char* file, unsigned char* data, int w, int h, int bps, int spp,
		   int xres, int yres);
};
