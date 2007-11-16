#include <math.h>

#include <iostream>
#include <iomanip>
#include <map>

#include "ArgumentList.hh"
#include "Codecs.hh"

#include "BarDecodeTokenizer.hh"
#include "BarDecodeScanner.hh"

using namespace Utility;

#ifdef BARDECODE_DEBUG
// not very efficient, yet effective
void PutPixel(Image& img, int x, int y, uint16_t R, uint16_t G,  uint16_t B)
{
  Image::iterator p=img.begin();
  p=p.at(x,y);
  p.setRGB(R, G, B);
  p.set(p);
}

void clear_image(Image& img)
{
    for (Image::iterator p = img.begin(); p != img.end(); ++p) {
        p.clear();
        p.set(p);
    }
    img.setRawData();
}

void Draw(Image& img, const FGMatrix& c, unsigned int r, unsigned int g, unsigned int b)
{
    for (unsigned int i=0; i<c.h; i++) {
        for (unsigned int j=0; j<c.w; j++) {
            if (c(j,i)) {
                PutPixel(img, j,i, r, g, b);
            } 
        }
    }
}

void Draw(Image& img, BarDecode::threshold_t threshold, unsigned int r, unsigned int g, unsigned int b)
{
    for (BarDecode::PixelIterator i(&img,threshold); ! i.end(); ++i) {
        if (*i) {
            PutPixel(img, i.get_x(),i.get_y(), r, g, b);
        } 
    }
}
#endif


using namespace BarDecode;

struct comp {
    bool operator() (const scanner_result_t& a, const scanner_result_t& b) const
    {
        if (a.type < b.type) return true;
        else if (a.type > b.type) return false;
        else return (a.code < b.code);
    }
};

int main (int argc, char* argv[])
{
  ArgumentList arglist (true); // enable residual gathering
  
  // setup the argument list
  Argument<bool> arg_help ("", "help",
			   "display this help text and exit");
  Argument<int> arg_threshold ("t", "threshold",
			       "bi-level threshold value", 0, 0, 1);

  arglist.Add (&arg_help);
  arglist.Add (&arg_threshold);

#ifdef BARDECODE_DEBUG
  Argument<std::string> arg_output ("o", "output", "output file", 0,1);
  arglist.Add (&arg_output);
#endif

  // parse the specified argument list - and maybe output the Usage
  if (!arglist.Read (argc, argv) || arg_help.Get() == true)
    {
      std::cerr << "barcode recognition module of the exact-image library"
                <<  " - Copyright 2007 by René Rebe, ExactCODE" << std::endl
                <<  " - Copyright 2007 by Lars Kuhtz, ExactCODE" << std::endl
                << "Usage:" << std::endl;
      
      arglist.Usage (std::cerr);
      return 1;
    }

  const std::vector<std::string>& filenames = arglist.Residuals();
  Image image;
  int errors = 0;
  bool multiple_files = filenames.size () > 1;
  
  for (std::vector<std::string>::const_iterator file = filenames.begin();
       file != filenames.end ();
       ++file)
    {
      if (!ImageCodec::Read (*file, image)) {
	std::cerr << "Error reading " << *file << std::endl;
	++errors;
      }
      
      // convert to 1-bit (threshold)
      int threshold = 0;
      if (arg_threshold.Get() != 0) {
	threshold = arg_threshold.Get();
      } else {
	threshold = 150;
      }
      
      std::map<scanner_result_t,int,comp> codes;
      BarDecode::BarcodeIterator it(&image,threshold,ean|code128|gs1_128|code39|code25i);
      while (! it.end() ) {
	++codes[*it];
	++it;
      }
      
      for (std::map<scanner_result_t,int>::const_iterator it = codes.begin();
	   it != codes.end();
	   ++it) {
	if (it->first.type&(ean|code128|gs1_128) || it->second > 1)
	  {
	    if (multiple_files)
	      std::cout << *file << ": ";
	    std::cout << it->first.code << " [type: " << it->first.type
		      << " at: (" << it->first.x << "," << it->first.y
		      << ")]" << std::endl;
	  }
      }
#ifdef BARDECODE_DEBUG
      if (arg_output.Get() != "") {
	Image o_image = image;
	Draw(o_image,threshold,0,0,255);
	
	if (!ImageCodec::Write(arg_output.Get(), o_image)) {
          std::cerr << "Error writing output file." << std::endl;
          return 1;
	}
      }
#endif
      if (codes.empty())
	++errors;
    }
  return errors;
}
