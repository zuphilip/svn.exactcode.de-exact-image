#include <math.h>

#include <iostream>
#include <iomanip>
#include <set>

#include "ArgumentList.hh"
#include "Codecs.hh"

#include "BarDecodeModulizer.hh"
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
  ArgumentList arglist;
  
  // setup the argument list
  Argument<bool> arg_help ("", "help",
			   "display this help text and exit");
  Argument<std::string> arg_input ("i", "input", "input file", 1, 1);

  Argument<int> arg_threshold ("t", "threshold",
			       "bi-level threshold value", 0, 0, 1);

  arglist.Add (&arg_help);
  arglist.Add (&arg_input);
  arglist.Add (&arg_threshold);

#ifdef BARDECODE_DEBUG
  Argument<std::string> arg_output ("o", "output", "output file", 0,1);
  arglist.Add (&arg_output);
#endif

  // parse the specified argument list - and maybe output the Usage
  if (!arglist.Read (argc, argv) || arg_help.Get() == true)
    {
      std::cerr << "barcode recognition module of the exact-image library"
                <<  " - Copyright 2007 - 2008 by René Rebe" << std::endl
                <<  " - Copyright 2007 - 2008 by Lars Kuhtz" << std::endl
                << "Usage:" << std::endl;
      
      arglist.Usage (std::cerr);
      return 1;
    }

  Image image;
  if (!ImageCodec::Read (arg_input.Get(), image)) {
    std::cerr << "Error reading input file." << std::endl;
    return 1;
  }

  // convert to 1-bit (threshold)
  int threshold = 0;
  if (arg_threshold.Get() != 0) {
    threshold = arg_threshold.Get();
  } else {
    threshold = 150;
  }

  std::set<scanner_result_t,comp> codes;
  BarDecode::BarcodeIterator it(&image,threshold,ean|code128|gs1_128|code39|code25i);
  while (! it.end() ) {
      codes.insert(*it);
      ++it;
  }

  for (std::set<scanner_result_t>::const_iterator it = codes.begin();
       it != codes.end();
       ++it) {
      std::cout << it->code << " [type: " << it->type << " at: (" << it->x << "," << it->y << ")]" << std::endl;
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
 
  return codes.empty();
}
