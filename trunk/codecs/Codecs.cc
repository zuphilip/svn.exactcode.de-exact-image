
#include "Codecs.hh"

#include <iostream>
#include <fstream>

std::vector<ImageCodec::loader_ref>* ImageCodec::loader = 0;

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
		       const std::string& codec)
{
  // TODO:
  bool bycodec = false;
  
  std::vector<loader_ref>::iterator it;
  for (it = loader->begin(); it != loader->end(); ++it)
    {
      if (it->ext == codec) {
	if (bycodec && !it->primary_entry)
	  continue;
	if (!bycodec && it->via_codec_only)
	  continue;
		
	if (it->loader->readImage (stream, image)) {
	  return true; 
	}
      }
    }
  
  std::cerr << "No suitable decoder found." << std::endl;
  return false;
}

bool ImageCodec::Write (std::ostream* stream, Image& image,
			const std::string& codec,
			int quality, const std::string& compress)
{
  // TODO:
  bool bycodec = false;
  
  std::vector<loader_ref>::iterator it;
  for (it = loader->begin(); it != loader->end(); ++it)
    {
      if (it->ext == codec) {
	if (bycodec && !it->primary_entry)
	  continue;
	if (!bycodec && it->via_codec_only)
	  continue;
	
	if (it->loader->writeImage (stream, image, quality, compress)) {
	  return true;
	}
      }
    }
  
  std::cerr << "No suitable encoder found." << std::endl;
}

// OLD API

bool ImageCodec::Read (std::string file, Image& image)
{
  std::string codec = get_codec (file);
  if (codec.empty())
    codec = get_ext (file);
  
  std::transform (codec.begin(), codec.end(), codec.begin(), tolower);
  
  std::istream* s;
  if (file != "-")
    s = new std::ifstream (file.c_str());
  else
    s = &std::cin;
  
  if (!*s) {
    std::cerr << "Can not open file " << file.c_str() << std::endl;
    return false;
  }
  
  return Read (s, image, codec);
}
  
bool ImageCodec::Write (std::string file, Image& image,
			int quality, const std::string& compress)
{
  std::string codec = get_codec (file);
  if (codec.empty())
    codec = get_ext (file);
  
  std::transform (codec.begin(), codec.end(), codec.begin(), tolower);
  
  std::ostream* s;
  if (file != "-")
    s = new std::ofstream (file.c_str());
  else
    s = &std::cout;
  
  if (!*s) {
    std::cerr << "Can not write file " << file.c_str() << std::endl;
    return false;
  }
  
  return Write (s, image, codec, quality, compress);
}
