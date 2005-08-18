
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <tiffconf.h>
#include <tiffio.h>

/* g3 for the CCITT Group 3 compression algorithm, -c  g4  for  the
              CCITT Group 4 compression algorithm, and -c lzw for Lempel-Ziv &
              Welch (the default).
 */

/*
                 compression = COMPRESSION_CCITTFAX3;
        } else if (streq(opt, "g4"))
                compression = COMPRESSION_CCITTFAX4;

 else if (strneq(opt, "lzw", 3)) {
                char* cp = strchr(opt, ':');
                if (cp)
                        predictor = atoi(cp+1);
                compression = COMPRESSION_LZW;
        } else if (strneq(opt, "zip", 3)) {
                char* cp = strchr(opt, ':');
                if (cp)
                        predictor = atoi(cp+1);
                compression = COMPRESSION_DEFLATE;

 */

#define RELEASE 1

void write_TIFF_file (const char* file, unsigned char* data, int w, int h, int bpp, int spp)
{
        TIFF *out;
	char thing [1024];
        unsigned char *inbuf, *outbuf;

	uint32 rowsperstrip = (uint32) -1;
	uint16 compression = COMPRESSION_LZW; // COMPRESSION_CCITTFAX4;

        out = TIFFOpen("test.tif", "w");
        if (out == NULL)
	  return;
        TIFFSetField(out, TIFFTAG_IMAGEWIDTH, w);
        TIFFSetField(out, TIFFTAG_IMAGELENGTH, h);
#ifdef RELEASE
        TIFFSetField(out, TIFFTAG_BITSPERSAMPLE, 1);// bpp);
	TIFFSetField(out, TIFFTAG_SAMPLESPERPIXEL, 1); // spp);
#else
        TIFFSetField(out, TIFFTAG_BITSPERSAMPLE, bpp);
	TIFFSetField(out, TIFFTAG_SAMPLESPERPIXEL, spp);
#endif
        TIFFSetField(out, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);

        TIFFSetField(out, TIFFTAG_COMPRESSION, compression);
        TIFFSetField(out, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
        TIFFSetField(out, TIFFTAG_IMAGEDESCRIPTION, "later some tag");
        TIFFSetField(out, TIFFTAG_SOFTWARE, "ExactImage");
        outbuf = (unsigned char *)_TIFFmalloc(TIFFScanlineSize(out));
        TIFFSetField(out, TIFFTAG_ROWSPERSTRIP,
        TIFFDefaultStripSize(out, rowsperstrip));
	
	{
	  int histogram [256] = { 0 };
	  for (int i = 0; i < h*w; i++)
	    histogram [data[i]] ++;
	  
	  int lowest = 255, highest = 0;
	  for (int i = 0; i <= 255; i++) {
	    printf ("%d: %d\n", i, histogram[i]);
	    if (histogram[i] > 5) { // 5 == magic denoise constant
	      if (i < lowest)
		lowest = i;
	      if (i > highest)
		highest = i;
	    }
	  }
	  printf ("lowest: %d - highest: %d\n", lowest, highest);
	  signed int a = (255 * 256) / (highest - lowest);
	  signed int b = -a * lowest;
	  
	  printf ("a: %f - b: %f\n", (float)a / 256, (float)b / 256);
	  for (int i = 0; i < h*w; i++)
	    data[i] = ((int) data[i] * a + b ) / 256;
	}
	
	for (uint32 row = 0; row < h; row++) {
#ifdef RELEASE
	  {
	    // convert to 1-bit (threshold)
	    unsigned char z = 0;
	    unsigned char* output = data;
	    unsigned char* input = data;
	    for (uint32 x = 0; x < w; x++) {
	      if (*input++ > 127)
		z = (z << 1) | 0x01;
	      else
		z <<= 1;
	      if (x % 8 == 7) {
		*output++ = z; z = 0;
	      }
	    }
	  }
#endif  
	  if (TIFFWriteScanline(out, data, row, 0) < 0)
	    break;
	  data += w*spp;
        }

        TIFFClose(out);
}
