
#include "ImageLoader.hh"

std::vector<ImageLoader::loader_ref>* ImageLoader::loader = 0;

std::string get_ext (const std::string& filename)
{
  // parse the filename extension
  std::string::size_type idx_ext = filename.rfind ('.');
  if (idx_ext && idx_ext != std::string::npos)
    return filename.substr (idx_ext + 1);
  else
    return "";
} 


bool ImageLoader::Read (const std::string& file, Image& image) {
  std::string ext = get_ext (file);
  std::vector<loader_ref>::iterator it;
  for (it = loader->begin(); it != loader->end(); ++it)
    {
      if (it->ext == ext)
	if (it->loader->readImage (file.c_str(), image))
	  return true; 
    }
  return false;
}
  
bool ImageLoader::Write (const std::string& file, Image& image) {
  std::string ext = get_ext (file);
  std::vector<loader_ref>::iterator it;
  for (it = loader->begin(); it != loader->end(); ++it)
    {
      if (it->ext == ext) {
	return it->loader->writeImage (file.c_str(), image);
      }
    }
  return false;
}
