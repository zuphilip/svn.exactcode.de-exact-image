
#ifndef IMAGELOADER_HH
#define IMAGELOADER_HH

#include <stdio.h>

#include <vector>
#include <iosfwd>

#include "Image.hh"

class ImageCodec
{
public:
  
  virtual ~ImageCodec () { };
  
  // NEW API, allowing the use of any STL i/o stream derived source
  static bool Read (std::istream* stream, Image& image,
		    const std::string& codec = "");
  static bool Write (std::ostream* stream, Image& image,
		     const std::string& codec = "",
		     int quality = 80, const std::string& compress = "");
  
  // OLD API, only left for compatibility
  // not const string& because the filename is parsed and changed internally
  static bool Read (std::string file, Image& image);
  static bool Write (std::string file, Image& image,
		     int quality = 80, const std::string& compress = "");
  
  // per codec methods
  virtual bool readImage (std::istream* stream, Image& image) = 0;
  virtual bool writeImage (std::ostream* stream, Image& image,
			   int quality, const std::string& compress) = 0;
  
protected:
  
  struct loader_ref {
    const char* ext;
    ImageCodec* loader;
    bool primary_entry;
    bool via_codec_only;
  };
  
  static std::vector<loader_ref>* loader;
  
  static void registerCodec (const char* _ext,
			      ImageCodec* _loader,
			      bool _via_codec_only = false)
  {
    static ImageCodec* last_loader;
    if (!loader)
      loader = new std::vector<loader_ref>;
    loader->push_back ( (loader_ref) {_ext, _loader,
			    _loader != last_loader, _via_codec_only}  );
    last_loader = _loader;
  }

  static void unregisterCodec (ImageCodec* _loader)
  {
    // TODO, remove
    if (loader->empty()) {
      delete loader;
      loader = 0;
    }
  }
  
};

#endif
