
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

  //  printf ("w: %d h: %d\n", *w, *h);
  //  printf ("spp: %d bps: %d stride: %d\n", *spp, *bps, stride);

  unsigned char* data = (unsigned char* ) malloc (stride * *h);
  
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
write_TIFF_file (const char *file, unsigned char *data, int w, int h,
		 int bps, int spp)
{
  TIFF *out;
  char thing[1024];
  unsigned char *inbuf, *outbuf;

  uint32 rowsperstrip = (uint32) - 1;
  uint16 compression = COMPRESSION_NONE;
  if (bps == 1)
    compression = COMPRESSION_CCITTFAX4;
  else
    compression = COMPRESSION_LZW;

  out = TIFFOpen (file, "w");
  if (out == NULL)
    return;
  TIFFSetField (out, TIFFTAG_IMAGEWIDTH, w);
  TIFFSetField (out, TIFFTAG_IMAGELENGTH, h);
  TIFFSetField (out, TIFFTAG_BITSPERSAMPLE, bps);
  TIFFSetField (out, TIFFTAG_SAMPLESPERPIXEL, spp);
  TIFFSetField (out, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);

  TIFFSetField (out, TIFFTAG_COMPRESSION, compression);
  if (bps == 1)
    TIFFSetField (out, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
  else if (spp == 1) // TODO: needs colormap ...
    TIFFSetField (out, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_PALETTE);
  else
    TIFFSetField (out, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
  
  TIFFSetField (out, TIFFTAG_IMAGEDESCRIPTION, "none");
  TIFFSetField (out, TIFFTAG_SOFTWARE, "ExactImage");
  outbuf = (unsigned char *) _TIFFmalloc (TIFFScanlineSize (out));
  TIFFSetField (out, TIFFTAG_ROWSPERSTRIP,
		TIFFDefaultStripSize (out, rowsperstrip));
  
  for (uint32 row = 0; row < h; row++)
    {
      if (TIFFWriteScanline (out, data, row, 0) < 0)
	break;
      data += (w * spp * bps) / 8;
    }

  TIFFClose (out);
}
