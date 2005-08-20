
#include <iostream>

#include "ArgumentList.hh"

#include "tiff.h"
#include "jpeg.h"

using namespace Utility;

int main (int argc, char* argv[])
{
  ArgumentList arglist;
  
  // setup the argument list
  Argument<bool> arg_help ("h", "help",
			   "display this help text and exit");
  Argument<std::string> arg_input ("i", "input", "input file",
                                   1, 1);
  Argument<std::string> arg_output ("o", "output", "output file",
				    1, 1);
  Argument<int> arg_low ("l", "low",
			 "low normalization value", 0, 0, 1);
  Argument<int> arg_high ("h", "high",
			  "high normalization value", 0, 0, 1);
  
  arglist.Add (&arg_help);
  arglist.Add (&arg_input);
  arglist.Add (&arg_output);
  arglist.Add (&arg_low);
  arglist.Add (&arg_high);
  
  // parse the specified argument list - and maybe output the Usage
  if (!arglist.Read (argc, argv) || arg_help.Get() == true)
    {
      std::cerr << "Color / Gray image to Bi-level optimizer"
                <<  " - Copyright 2005 by René Rebe" << std::endl
                << "Usage:" << std::endl;
      
      arglist.Usage (std::cout);
      return 1;
    }
  
  int w, h, bps, spp;
  unsigned char* data = read_JPEG_file (arg_input.Get().c_str(),
					&w, &h, &bps, &spp);
  if (!data)
  {
    std::cerr << "Error reading JPEG." << std::endl;
    return 1;
  }
  
  // convert to RGB to gray - TODO: more cases
  if (spp == 3 && bps == 8) {
    std::cerr << "RGB -> Gray convertion" << std::endl;
    
    unsigned char* output = data;
    unsigned char* input = data;
    
    for (int i = 0; i < w*h; i++)
      {
	// R G B order and associated weighting
	int c = (int)input [0] * 28;
	c += (int)input [1] * 59;
	c += (int)input [2] * 11;
	input += 3;
	
	*output++ = (unsigned char)(c / 100);
	
	spp = 1; // converted data right now
      }
  }
  else if (spp != 1 && bps != 8)
    {
      std::cerr << "Can't yet handle " << spp << " samples with "
		<< bps << " bits per sample." << std::endl;
      return 1;
    }
  
  {
    int histogram[256] = { 0 };
    for (int i = 0; i < h * w; i++)
      histogram[data[i]]++;

    int lowest = 255, highest = 0;
    for (int i = 0; i <= 255; i++)
      {
	// printf ("%d: %d\n", i, histogram[i]);
	if (histogram[i] > 2) // 5 == magic denoise constant
	  {
	    if (i < lowest)
	      lowest = i;
	    if (i > highest)
	      highest = i;
	  }
      }
    std::cerr << "lowest: " << lowest << " - highest: "
	      << highest << std::endl;
    
    if (arg_low.Get() != 0) {
      lowest = arg_low.Get();
      std::cerr << "Low value overwritten: " << lowest << std::endl;
    }
    
    if (arg_high.Get() != 0) {
      highest = arg_high.Get();
      std::cerr << "High value overwritten: " << highest << std::endl;
    }
    
    // TODO use options
    signed int a = (255 * 256) / (highest - lowest);
    signed int b = -a * lowest;

    std::cerr << "a: " << (float) a / 256
	      << " b: " << (float) b / 256 << std::endl;
    for (int i = 0; i < h * w; i++)
      data[i] = ((int) data[i] * a + b) / 256;
  }

  // Convolution Matrix (unsharp mask a-like)
  unsigned char *data2 = (unsigned char *) malloc (w * h);
  {
    // any matrix and devisior
#define matrix_w 5
#define matrix_h 5

#define matrix_w2 ((matrix_w-1)/2)
#define matrix_h2 ((matrix_h-1)/2)

    typedef float matrix_type;

    matrix_type matrix[matrix_w][matrix_h] = {
      {0, 0, -1, 0, 0},
      {0, -8, -21, -8, 0},
      {-1, -21, 299, -21, -1},
      {0, -8, -21, -8, 0},
      {0, 0, -1, 0, 0},
    };

    matrix_type divisor = 179;

    for (int y = 0; y < h; y++)
      {
	for (int x = 0; x < w; x++)
	  {
	    // for now copy border pixels
	    if (y < matrix_h2 || y > h - matrix_h2 ||
		x < matrix_w2 || x > w - matrix_w2)
	      data2[x + y * w] = data[x + y * w];
	    else
	      {
		matrix_type sum = 0;
		for (int x2 = 0; x2 < matrix_w; x2++)
		  {
		    for (int y2 = 0; y2 < matrix_h; y2++)
		      {
			matrix_type v = data[x - matrix_w2 + x2 +
					     ((y - matrix_h2 + y2) * w)];
			sum += v * matrix[x2][y2];
		      }
		  }
		
		sum /= divisor;
		unsigned char z = (unsigned char)
		  (sum > 255 ? 255 : sum < 0 ? 0 : sum);
		data2[x + y * w] = z;
	      }
	  }
      }
  }
  data = data2;

  // convert to 1-bit (threshold)
  
  unsigned char *output = data;
  unsigned char *input = data;
  for (int row = 0; row < h; row++)
    {
      unsigned char z = 0;
      int x = 0;
      for (; x < w; x++)
	{
	  if (*input++ > 127)
	    z = (z << 1) | 0x01;
	  else
	    z <<= 1;
	  if (x % 8 == 7)
	    {
	      *output++ = z;
	      z = 0;
	    }
	}
      // remainder - TODO: test for correctness ...
      int remainder = 8 - x % 8;
      if (remainder != 8)
	{
	  z <<= remainder;
	  *output++ = z;
	}
    }

  // new image data - and 8 pixel align due to packing nature
  w = ((w + 7) / 8) * 8;
  bps = 1;
  
  write_TIFF_file (arg_output.Get().c_str(), data, w, h, bps, spp);
  
  free (data2);

  return 0;
}
