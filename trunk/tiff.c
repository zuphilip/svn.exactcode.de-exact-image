
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <tiffconf.h>
#include <tiffio.h>

#include "tiff.h"

#define RELEASE

unsigned char*
read_TIFF_file (const char *file, int* w, int* h, int* bps, int* spp)
{
  TIFF* in;
  in = TIFFOpen(file, "r");
  if (!in)
    return 0;
  
  uint16 photometric = 0;
  TIFFGetField(in, TIFFTAG_PHOTOMETRIC, &photometric);
  printf ("photometric: %d\n", photometric);
  switch (photometric)
    {
    case PHOTOMETRIC_MINISWHITE:
    case PHOTOMETRIC_MINISBLACK:
    case PHOTOMETRIC_RGB:
    case PHOTOMETRIC_PALETTE:
      break;
    
    default:
      printf("Bad photometric: %d\n", photometric);
      return 0;
    }
  
  TIFFGetField(in, TIFFTAG_IMAGEWIDTH, w);
  TIFFGetField(in, TIFFTAG_IMAGELENGTH, h);
  
  uint16 _spp;
  TIFFGetField(in, TIFFTAG_SAMPLESPERPIXEL, &_spp); *spp = _spp;
  
  uint16 _bps;
  TIFFGetField(in, TIFFTAG_BITSPERSAMPLE, &_bps); *bps = _bps;
  
  uint16 config;
  TIFFGetField(in, TIFFTAG_PLANARCONFIG, &config);

  // format we know about

  int stride = (((*w) * (*spp) * (*bps)) + 7) / 8;

  printf ("w: %d h: %d\n", *w, *h);
  printf ("spp: %d bps: %d stride: %d\n", *spp, *bps, stride);

  unsigned char* data = malloc (stride * *h);
  
  unsigned char* data2 = data;
  for (unsigned int row = 0; row < *h; row++)
    {
      if (TIFFReadScanline(in, data2, row, 0) < 0)
	break;
      data2 += stride;
    }
  return data;
}

void
write_TIFF_file (const char *file, unsigned char *data, int w, int h, int bpp,
		 int spp)
{
  TIFF *out;
  char thing[1024];
  unsigned char *inbuf, *outbuf;

  uint32 rowsperstrip = (uint32) - 1;
  uint16 compression = COMPRESSION_CCITTFAX4; // COMPRESSION_LZW

  out = TIFFOpen (file, "w");
  if (out == NULL)
    return;
  TIFFSetField (out, TIFFTAG_IMAGEWIDTH, w);
  TIFFSetField (out, TIFFTAG_IMAGELENGTH, h);
  TIFFSetField (out, TIFFTAG_BITSPERSAMPLE, 1);	// bpp);
  TIFFSetField (out, TIFFTAG_SAMPLESPERPIXEL, 1);	// spp);
  TIFFSetField (out, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);

  TIFFSetField (out, TIFFTAG_COMPRESSION, compression);
  TIFFSetField (out, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
  TIFFSetField (out, TIFFTAG_IMAGEDESCRIPTION, "later some tag");
  TIFFSetField (out, TIFFTAG_SOFTWARE, "ExactImage");
  outbuf = (unsigned char *) _TIFFmalloc (TIFFScanlineSize (out));
  TIFFSetField (out, TIFFTAG_ROWSPERSTRIP,
		TIFFDefaultStripSize (out, rowsperstrip));
  
  // convert to RGB to gray - TODO: more cases
  if (spp == 3 && bpp == 8) {
    printf ("RGB -> Gray convertion\n");
    unsigned char *output = data;
    unsigned char *input = data;
    
    for (int i = 0; i < w*h; i++)
      {
	// R G B order
	int c = (int)input [0] * 28;
	c += (int)input [1] * 59;
	c += (int)input [2] * 11;
	input += 3;
	
	*output++ = (unsigned char)(c / 100);
	
	spp = 1; // converted data right now
      }
  }
  else if (spp != 1 && bpp != 8)
    {
      printf ("Can't yet handle %d samples with %d bits per pixel\n.", spp, bpp);
      exit (1);
    }
  
  {
    int histogram[256] = { 0 };
    for (int i = 0; i < h * w; i++)
      histogram[data[i]]++;

    int lowest = 255, highest = 0;
    for (int i = 0; i <= 255; i++)
      {
	printf ("%d: %d\n", i, histogram[i]);
	if (histogram[i] > 2) // 5 == magic denoise constant
	  {
	    if (i < lowest)
	      lowest = i;
	    if (i > highest)
	      highest = i;
	  }
      }
    printf ("lowest: %d - highest: %d\n", lowest, highest);
    signed int a = (255 * 256) / (highest - lowest);
    signed int b = -a * lowest;

    printf ("a: %f - b: %f\n", (float) a / 256, (float) b / 256);
    for (int i = 0; i < h * w; i++)
      data[i] = ((int) data[i] * a + b) / 256;
  }

  // Convolution Matrix (unsharp mask like)
  unsigned char *data2 = malloc (w * h);
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
		unsigned char z = sum > 255 ? 255 : sum < 0 ? 0 : sum;
		data2[x + y * w] = z;
	      }
	  }
      }
    // copy unfiltered 
  }
  data = data2;

  for (uint32 row = 0; row < h; row++)
    {
      {
	// convert to 1-bit (threshold)
	unsigned char z = 0;
	unsigned char *output = data;
	unsigned char *input = data;
	uint32 x = 0;
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

      if (TIFFWriteScanline (out, data, row, 0) < 0)
	break;
      data += w * spp;
    }

  free (data2);

  TIFFClose (out);
}
