#include <math.h>

#include <iostream>
#include <iomanip>
#include <set>

#include "ArgumentList.hh"
#include "Codecs.hh"
#include "Colorspace.hh"
#include "Matrix.hh"
#include "FG-Matrix.hh"
#include "optimize2bw.hh"

#include "BarDecodeModulizer.hh"
#include "BarDecodeScanner.hh"


using namespace Utility;


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


int main (int argc, char* argv[])
{
  ArgumentList arglist;
  
  // setup the argument list
  Argument<bool> arg_help ("", "help",
			   "display this help text and exit");
  Argument<std::string> arg_input ("i", "input", "input file", 1, 1);

  Argument<std::string> arg_output ("o", "output", "output file", 0,1);

  // optimize2bw options

  Argument<int> arg_low ("l", "low",
			 "low normalization value", 0, 0, 1);
  Argument<int> arg_high ("h", "high",
			  "high normalization value", 0, 0, 1);
  
  Argument<int> arg_threshold ("t", "threshold",
			       "bi-level threshold value", 0, 0, 1);

  Argument<int> arg_radius ("r", "radius",
			    "\"unsharp mask\" radius", 0, 0, 1);

  arglist.Add (&arg_help);
  arglist.Add (&arg_input);
  arglist.Add (&arg_output);
  arglist.Add (&arg_low);
  arglist.Add (&arg_high);
  arglist.Add (&arg_threshold);
  arglist.Add (&arg_radius);


  // parse the specified argument list - and maybe output the Usage
  if (!arglist.Read (argc, argv) || arg_help.Get() == true)
    {
      std::cerr << "Based on Color / Gray image to Bi-level optimizer"
                <<  " - Copyright 2005, 2006 by René Rebe" << std::endl
                << "Usage:" << std::endl;
      
      arglist.Usage (std::cerr);
      return 1;
    }

  Image image;
  if (!ImageCodec::Read (arg_input.Get(), image)) {
    std::cerr << "Error reading input file." << std::endl;
    return 1;
  }

  int low = 0;
  int high = 170; // seems a god choice
  int sloppy_threshold = 0;
  int radius = 3;
  double sd = 2.1;
  
  if (arg_low.Get() != 0) {
    low = arg_low.Get();
    std::cerr << "Low value overwritten: " << low << std::endl;
  }
  
  if (arg_high.Get() != 0) {
    high = arg_high.Get();
    std::cerr << "High value overwritten: " << high << std::endl;
  }
  
  if (arg_radius.Get() != 0) {
    radius = arg_radius.Get();
    std::cerr << "Radius: " << radius << std::endl;
  }
  
  // convert to 1-bit (threshold)
  int threshold = 0;
  
  if (arg_threshold.Get() != 0) {
    threshold = arg_threshold.Get();
  }

  if (arg_threshold.Get() == 0)
    threshold = 170;

  std::set<std::string> codes;
  BarDecode::BarCodeIterator it(&image,threshold);
  while (! it.end() ) {
      codes.insert(*it);
      ++it;
  }

  for (std::set<std::string>::const_iterator it = codes.begin();
       it != codes.end();
       ++it) {
      std::cout << *it << std::endl;
  }

  if (arg_output.Get() != "") {
      Image o_image = image;
      Draw(o_image,threshold,0,0,255);

      if (!ImageCodec::Write(arg_output.Get(), o_image)) {
          std::cerr << "Error writing output file." << std::endl;
          return 1;
      }
  }
 
  return codes.empty();
}
