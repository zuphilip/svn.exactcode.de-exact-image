
#ifndef IMAGELOADER_HH
#define IMAGELOADER_HH

#include <vector>
#include <iostream>

#include "Image.hh"

inline std::string get_ext (const std::string& filename)
{
  // parse the filename extension
  std::string::size_type idx_ext = filename.rfind ('.');
  if (idx_ext && idx_ext != std::string::npos)
    return filename.substr (idx_ext + 1);
  else
    return "";
} 

class ImageLoader
{
public:
  
  virtual ~ImageLoader () {};
  
  virtual unsigned char*
  readImage (const char* file, int* w, int* h, int* bps, int* spp,
	     int* xres, int* yres) = 0;
  
  virtual void
  writeImage (const char* file, unsigned char* data, int w, int h,
	      int bps, int spp, int xres, int yres) = 0;
  
  static bool Read (const std::string& file, Image& image) {
    std::string ext = get_ext (file);
    std::vector<loader_ref>::iterator it;
    for (it = loader->begin(); it != loader->end(); ++it)
      {
	 if (it->ext == ext)
	  image.data = it->loader->readImage (file.c_str(), &image.w, &image.h,
					      &image.bps, &image.spp, &image.xres, &image.yres);
	if (image.data)
	  return true; 
      }
    return false;
  }
  
  static bool Write (const std::string& file, Image& image) {
    std::string ext = get_ext (file);
    std::vector<loader_ref>::iterator it;
    for (it = loader->begin(); it != loader->end(); ++it)
      {
	if (it->ext == ext) {
	  it->loader->writeImage (file.c_str(), image.data, image.w, image.h,
				  image.bps, image.spp, image.xres, image.yres);
	  return true;
	}
      }
    return false;
  }
  
protected:
  
  struct loader_ref {
    const char* ext;
    ImageLoader* loader;
  };
  
  static std::vector<loader_ref>* loader;
  
  static void registerLoader (const char* _ext, ImageLoader* _loader)
  {
    if (!loader)
      loader = new std::vector<loader_ref>;
    loader->push_back ( (loader_ref) {_ext, _loader}  );
  }

  static void unregisterLoader (ImageLoader* _loader)
  {
    // TODO, remove
    if (loader->empty()) {
      delete loader;
      loader = 0;
    }
  }
  
};

#endif