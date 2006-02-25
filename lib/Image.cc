
#include <string>
#include <iostream>

#include "Image.hh"

#include "tiff.hh"
#include "jpeg.hh"
#include "jpeg2000.hh"
#include "png.hh"
#include "bmp.hh"

std::string get_ext (const std::string& filename)
{
  // parse the filename extension
  std::string::size_type idx_ext = filename.rfind ('.');
  if (idx_ext && idx_ext != std::string::npos)
    return filename.substr (idx_ext + 1);
  else
    return "";
}
  
bool Image::Read (const std::string& file) {
    
    std::string ext = get_ext (file);
    
    if (ext == "tif")
      data = read_TIFF_file (file.c_str(), &w, &h, &bps, &spp, &xres, &yres);
    else if (ext == "jpg")
      data = read_JPEG_file (file.c_str(), &w, &h, &bps, &spp, &xres, &yres);
    else if (ext == "jp2")
      data = read_JPEG2000_file (file.c_str(), &w, &h, &bps, &spp, &xres, &yres);
    else if (ext == "png")
      data = read_PNG_file (file.c_str(), &w, &h, &bps, &spp, &xres, &yres);
    else if (ext == "bmp")
      data = read_BMP_file (file.c_str(), &w, &h, &bps, &spp, &xres, &yres);
    else {
      std::cerr << "Unrecognized extension: " << ext << std::endl;
      return false;
    }
	    
    return true;
  }

bool Image::Write (const std::string& file) {
    std::string ext = get_ext (file);
    
    if (ext == "tif")
      write_TIFF_file (file.c_str(), data, w, h, bps, spp, xres, yres);
    else if (ext == "jpg")
      write_JPEG_file (file.c_str(), data, w, h, bps, spp, xres, yres);
    else if (ext == "jp2")
      write_JPEG2000_file (file.c_str(), data, w, h, bps, spp, xres, yres);
    else if (ext == "png")
      write_PNG_file (file.c_str(), data, w, h, bps, spp, xres, yres);
    else if (ext == "bmp")
      write_BMP_file (file.c_str(), data, w, h, bps, spp, xres, yres);
    else {
      std::cerr << "Unrecognized extension: " << ext << std::endl;
      return false;
    }
    
    return true;
}
