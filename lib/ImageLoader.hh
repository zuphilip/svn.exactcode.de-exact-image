
#ifndef IMAGELOADER_HH
#define IMAGELOADER_HH

#include <stdio.h>

#include <vector>
#include <iostream>

#include "Image.hh"

class ImageLoader
{
public:
  
  virtual ~ImageLoader () { };
  
  // not const string& because the filename is parsed and changed internally
  static bool Read (std::string file, Image& image);
  static bool Write (std::string file, Image& image);

  virtual bool readImage (FILE* file, Image& image) = 0;
  virtual bool writeImage (FILE* file, Image& image) = 0;
  
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
