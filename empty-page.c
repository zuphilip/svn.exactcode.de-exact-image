
#include <stdio.h>

#include "tiff.h"
#include "jpeg.h"

#include "ArgumentList.hh"

using namespace Utility;

int main (int argc, char* argv[])
{
  ArgumentList arglist;
  
  // setup the argument list
  Argument<bool> arg_help ("h", "help",
			   "display this help text and exit");
  Argument<std::string> arg_input ("i", "input", "input file",
                                   1, 1);
  Argument<int> arg_margin ("m", "margin",
			    "border margin to skip", 16, 0, 1);
  Argument<float> arg_percent ("p", "percentage",
			       "coverate for non-empty page", 0.025, 0, 1);
  
  arglist.Add (&arg_help);
  arglist.Add (&arg_input);
  arglist.Add (&arg_margin);
  arglist.Add (&arg_percent);
  
  // parse the specified argument list - and maybe output the Usage
  if (!arglist.Read (argc, argv) || arg_help.Get() == true)
    {
      std::cout << "Empty page detector"
                <<  " - Copyright 2005 by Renÿ Rebe" << std::endl
                << "Usage:" << std::endl;

      arglist.Usage (std::cout);
      return 1;
    }
  
  int w, h, bps, spp;
  unsigned char* data = read_TIFF_file (arg_input.Get().c_str(),
					&w, &h, &bps, &spp);
  if (!data)
  {
    printf ("Error reading TIFF.\n");
    return 1;
  }
  printf ("read TIFF: w: %d: h: %d bps: %d spp: %d\n", w, h, bps, spp);
  
  // if not 1-bit optimize
  if (spp != 1 || bps != 1)
    {
      printf ("the image type needs optimzation - to be added\n");
      
      return 1;
    }
  
  // count bits and decide based on that
  
  // create a bit table for fast lookup
  int bits_set[256] = { 0 };
  for (int i = 0; i < 256; i++) {
    int bits = 0;
    for (int j = i; j != 0; j >>= 1) {
      bits += (j & 0x01);
    }
    bits_set[i] = bits;
  }
  
  int stride = (w * bps * spp + 7) / 8;
  
  int margin = 4;
  
  // count pixels
  int pixels = 0;
  for (int row = margin; row < h-margin; row++) {
    for (int x = margin; x < stride - margin; x++) {
      int b = bits_set [ data[stride*row + x] ];
      // it is a bits_set table - and we want tze zeros ...
      pixels += 8-b;
    }
  }
  
  float percentage = (float)pixels/(w*h) * 100;
  printf ("the image has %d dark pixels from a total of %d (%f%%)\n",
	  pixels, w*h, percentage );
  
  if (percentage > 0.05)
    printf ("non-empty");
  else
    printf ("empty");
  
  FILE* f = fopen ("tiff-load.raw", "w+");
  fwrite (data, stride * h, 1, f);
  fclose(f);
  
  return 0;
}
