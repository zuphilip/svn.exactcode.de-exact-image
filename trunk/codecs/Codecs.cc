
#include "Codecs.hh"

#include <iostream>
#include <fstream>

std::vector<ImageCodec::loader_ref>* ImageCodec::loader = 0;


ImageCodec::~ImageCodec ()
{
  unregisterCodec (this);
}

std::string get_ext (const std::string& filename)
{
  // parse the filename extension
  std::string::size_type idx_ext = filename.rfind ('.');
  if (idx_ext && idx_ext != std::string::npos)
    return filename.substr (idx_ext + 1);
  else
    return "";
} 

std::string get_codec (std::string& filename)
{
  // parse the filename extension
  std::string::size_type idx_colon = filename.find (':');
  if (idx_colon && idx_colon != std::string::npos) {
    std::string codec = filename.substr (0, idx_colon);
    filename.erase (0, idx_colon+1);
    return codec;
  }
  else
    return "";
} 

// NEW API

bool ImageCodec::Read (std::istream* stream, Image& image,
		       std::string codec)
{
  std::transform (codec.begin(), codec.end(), codec.begin(), tolower);
  
  std::vector<loader_ref>::iterator it;
  for (it = loader->begin(); it != loader->end(); ++it)
    {
      if (codec.empty()) // try via magic
	{
	  // use primary entry to only try each codec once
	  if (it->primary_entry && !it->via_codec_only) {
	    if (it->loader->readImage (stream, image)) {
	      image.setDecoderID (it->loader->getID ());
	      return true;
	    }
	    // TODO: remove once the codecs are clean
	    stream->clear ();
	    stream->seekg (0);
	  }
	}
      else // manual codec spec
	{
	  if (it->primary_entry && it->ext == codec) {
	    return it->loader->readImage (stream, image);
	  }
	}
    }
  
  std::cerr << "No matching codec found." << std::endl;
  return false;
}

bool ImageCodec::Write (std::ostream* stream, Image& image,
			std::string codec, std::string ext,
			int quality, const std::string& compress)
{
  std::transform (codec.begin(), codec.end(), codec.begin(), tolower);
  std::transform (ext.begin(), ext.end(), ext.begin(), tolower);
  

  std::vector<loader_ref>::iterator it;
  for (it = loader->begin(); it != loader->end(); ++it)
    {
      if (codec.empty()) // match extension
	{
	  if (it->ext == ext)
	    return (it->loader->writeImage (stream, image, quality, compress));
	}
      else // manual codec spec
	{
	  if (it->primary_entry && it->ext == codec) {
	    return (it->loader->writeImage (stream, image, quality, compress));
	  }
	}
    }
  
  std::cerr << "No matching codec found." << std::endl;
  return false;
}

// OLD API

bool ImageCodec::Read (std::string file, Image& image)
{
  std::string codec = get_codec (file);
  
  std::istream* s;
  if (file != "-")
    s = new std::ifstream (file.c_str());
  else
    s = &std::cin;
  
  if (!*s) {
    std::cerr << "Can not open file " << file.c_str() << std::endl;
    return false;
  }
  
  bool res = Read (s, image, codec);
  if (s != &std::cin)
    delete s;
  return res;
}
  
bool ImageCodec::Write (std::string file, Image& image,
			int quality, const std::string& compress)
{
  std::string codec = get_codec (file);
  std::string ext = get_ext (file);
  
  std::ostream* s;
  if (file != "-")
    s = new std::ofstream (file.c_str());
  else
    s = &std::cout;
  
  if (!*s) {
    std::cerr << "Can not write file " << file.c_str() << std::endl;
    return false;
  }
  
  bool res = Write (s, image, codec, ext, quality, compress);
  if (s != &std::cout)
    delete s;
  return res;
}

void ImageCodec::registerCodec (const char* _ext, ImageCodec* _loader,
				bool _via_codec_only)
{
  static ImageCodec* last_loader = 0;
  if (!loader)
    loader = new std::vector<loader_ref>;
  
  loader->push_back ( (loader_ref) {_ext, _loader,
			  _loader != last_loader, _via_codec_only} );
  last_loader = _loader;
}

void ImageCodec::unregisterCodec (ImageCodec* _loader)
{
  // sanity check
  if (!loader) {
    std::cerr << "unregisterCodec: no codecs, unregister impossible!" << std::endl;
  }
  
  // remove from array
  std::vector<loader_ref>::iterator it;
  for (it = loader->begin(); it != loader->end();)
    if (it->loader == _loader)
      it = loader->erase (it);
    else
      ++it;
  
  if (loader->empty()) {
    delete loader;
    loader = 0;
  }
}
