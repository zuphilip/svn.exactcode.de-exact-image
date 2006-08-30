
#include "Codecs.hh"

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

bool ImageLoader::Read (std::string file, Image& image) {
  bool bycodec = true;
  
  std::string codec = get_codec (file);
  if (codec.empty()) {
    codec = get_ext (file);
    bycodec = false;
  }
  std::transform (codec.begin(), codec.end(), codec.begin(), tolower);
  
  FILE* fp;
  if (file != "-")
    fp = fopen (file.c_str(), "r");
  else
    fp = stdin;
  
  if (!fp) {
    std::cerr << "Can not open file " << file.c_str() << std::endl;
    return false;
  }
  
  std::vector<loader_ref>::iterator it;
  for (it = loader->begin(); it != loader->end(); ++it)
    {
      if (it->ext == codec) {
	if (bycodec && !it->primary_entry)
	  continue;
	if (!bycodec && it->via_codec_only)
	  continue;
		
	if (it->loader->readImage (fp, image)) {
	  if (file != "-")
	    fclose (fp);
	  return true; 
	}
      }
    }
  if (file != "-")
    fclose (fp);
  std::cerr << "No suitable decoder found for: " << file.c_str() << std::endl;
  return false;
}
  
bool ImageLoader::Write (std::string file, Image& image,
			 int quality, const std::string& compress) {
  bool bycodec = true;

  std::string codec = get_codec (file);
  if (codec.empty()) {
    codec = get_ext (file);
    bycodec = false;
  }
  
  std::transform (codec.begin(), codec.end(), codec.begin(), tolower);
  
  FILE* fp;
  if (file != "-")
    fp = fopen (file.c_str(), "w");
  else
    fp = stdout;
  
  if (!fp) {
    std::cerr << "Can not write file " << file.c_str() << std::endl;
    return false;
  }
  
  std::vector<loader_ref>::iterator it;
  for (it = loader->begin(); it != loader->end(); ++it)
    {
      if (it->ext == codec) {
	if (bycodec && !it->primary_entry)
	  continue;
	if (!bycodec && it->via_codec_only)
	  continue;
	
	if (it->loader->writeImage (fp, image, quality, compress)) {
	  if (file != "-")
	    fclose (fp);
	  return true;
	}
      }
    }
  if (file != "-")
    fclose (fp);
  std::cerr << "No suitable encoder found for: " << file.c_str() << std::endl;
  return false;
}
