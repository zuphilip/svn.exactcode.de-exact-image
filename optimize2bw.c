
#include <math.h>

#include <iostream>
#include <iomanip>

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
  
  Argument<int> arg_threshold ("t", "threshold",
			       "threshold", 0, 0, 1);

  Argument<int> arg_radius ("r", "radius",
			    "\"unsharp mask\" radius", 0, 0, 1);
  
  arglist.Add (&arg_help);
  arglist.Add (&arg_input);
  arglist.Add (&arg_output);
  arglist.Add (&arg_low);
  arglist.Add (&arg_high);
  arglist.Add (&arg_threshold);
  
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
	std::cout << i << ": "<< histogram[i] << std::endl;
	if (histogram[i] > 2) // magic denoise constant
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

    typedef float matrix_type;

    /*    const matrix_type matrix[matrix_w][matrix_h] = {
      {0, 0, -1, 0, 0},
      {0, -8, -21, -8, 0},
      {-1, -21, 299, -21, -1},
      {0, -8, -21, -8, 0},
      {0, 0, -1, 0, 0},
      };*/

    // compute kernel (convolution matrix to move over the iamge)
    
    int radius = 2;
    int width = radius * 2 + 1;
    matrix_type divisor = 1;
    float sd = 0.9;
    
    if (arg_radius.Get() != 0) {
      radius = arg_radius.Get();
      std::cerr << "Radius: " << radius << std::endl;
    }
    
    matrix_type *matrix = new matrix_type[width * width];
    
    std::cout << std::fixed << std::setprecision(2);
    for (int y = -radius; y <= radius; y++) {
      for (int x = -radius; x <= radius; x++) {
	double v = - exp (-((float)x*x + (float)y*y) / ((float)2 * sd * sd));
	if (x == 0 && y == 0)
	  v *= -5;
	std::cout << v << " ";
	matrix[x + radius + (y+radius)*width] = v;
      }
      std::cout << std::endl;
    }
    
    #define KernelRank 3

#ifdef image_magick_reference
    long bias;

    MagickRealType alpha,  normalize;
    
    register long i;
    
    /* Generate a 1-D convolution matrix. Calculate the kernel at higher
       resolution than needed and average the results as a form of numerical
       integration to get the best accuracy. */

    assert(sigma != 0.0);
    if (width < 3)
      width=3;
    if ((width & 0x01) == 0)
      width++;
    bias=KernelRank*(long) width/2;
    for (i=(-bias); i <= bias; i++)
      {
	alpha=exp( (-((double) (i*i))/(2.0*KernelRank*KernelRank*sigma*sigma))
		   );
	(*kernel)[(i+bias)/KernelRank]+=alpha/(MagickSQ2PI*sigma);
      }
    normalize=0.0;
    for (i=0; i < (long) width; i++)
      normalize+=(*kernel)[i];
    for (i=0; i < (long) width; i++)
      (*kernel)[i]/=normalize;
    return(width);
    
#endif
    
    for (int y = 0; y < h; y++)
      {
	for (int x = 0; x < w; x++)
	  {
	    // for now copy border pixels
	    if (y < radius || y > h - radius ||
		x < radius || x > w - radius)
	      data2[x + y * w] = data[x + y * w];
	    else
	      {
		matrix_type sum = 0;
		for (int x2 = 0; x2 < width; x2++)
		  {
		    for (int y2 = 0; y2 < width; y2++)
		      {
			matrix_type v = data[x - radius + x2 +
					     ((y - radius + y2) * w)];
			sum += v * matrix[x2 + y2*width];
			if (y == h/2 && x == w/2)
			  std::cout << sum << std::endl;
		      }
		  }
		
		
		sum /= divisor;
		if (y == h/2 && x == w/2)
		  std::cout << sum << std::endl;
		unsigned char z = (unsigned char)
		  (sum > 255 ? 255 : sum < 0 ? 0 : sum);
		data2[x + y * w] = z;
	      }
	  }
      }
  }
  data = data2;
  
// #define DEBUG
#ifdef DEBUG
  FILE* f = fopen ("optimized.raw", "w+");
  fwrite (data, w * h, 1, f);
  fclose(f);
#endif
  
  // convert to 1-bit (threshold)
  
  unsigned char *output = data;
  unsigned char *input = data;
  
  int threshold = 127;
    
  if (arg_threshold.Get() != 0) {
    threshold = arg_threshold.Get();
    std::cerr << "Threshold: " << threshold << std::endl;
  }
    
  for (int row = 0; row < h; row++)
    {
      unsigned char z = 0;
      int x = 0;
      for (; x < w; x++)
	{
	  z <<= 1;
	  if (*input++ > threshold)
	    z |= 0x01;
	  
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
