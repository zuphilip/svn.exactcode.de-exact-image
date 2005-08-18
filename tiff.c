
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

// #define RELEASE

void
write_TIFF_file (const char *file, unsigned char *data, int w, int h, int bpp,
		 int spp)
{
  TIFF *out;
  char thing[1024];
  unsigned char *inbuf, *outbuf;

  uint32 rowsperstrip = (uint32) - 1;
  uint16 compression = COMPRESSION_LZW;	// COMPRESSION_CCITTFAX4;

  out = TIFFOpen ("test.tif", "w");
  if (out == NULL)
    return;
  TIFFSetField (out, TIFFTAG_IMAGEWIDTH, w);
  TIFFSetField (out, TIFFTAG_IMAGELENGTH, h);
#ifdef RELEASE
  TIFFSetField (out, TIFFTAG_BITSPERSAMPLE, 1);	// bpp);
  TIFFSetField (out, TIFFTAG_SAMPLESPERPIXEL, 1);	// spp);
#else
  TIFFSetField (out, TIFFTAG_BITSPERSAMPLE, bpp);
  TIFFSetField (out, TIFFTAG_SAMPLESPERPIXEL, spp);
#endif
  TIFFSetField (out, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);

  TIFFSetField (out, TIFFTAG_COMPRESSION, compression);
  TIFFSetField (out, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
  TIFFSetField (out, TIFFTAG_IMAGEDESCRIPTION, "later some tag");
  TIFFSetField (out, TIFFTAG_SOFTWARE, "ExactImage");
  outbuf = (unsigned char *) _TIFFmalloc (TIFFScanlineSize (out));
  TIFFSetField (out, TIFFTAG_ROWSPERSTRIP,
		TIFFDefaultStripSize (out, rowsperstrip));

  {
    int histogram[256] = { 0 };
    for (int i = 0; i < h * w; i++)
      histogram[data[i]]++;

    int lowest = 255, highest = 0;
    for (int i = 0; i <= 255; i++)
      {
	printf ("%d: %d\n", i, histogram[i]);
	if (histogram[i] > 5)
	  {			// 5 == magic denoise constant
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
#define WORKS
#ifdef WORKS

#ifdef RIGHT
#define matrix_w 5
#define matrix_h 5
#else
#define matrix_w 3
#define matrix_h 3
#endif

#define matrix_w2 ((matrix_w-1)/2)
#define matrix_h2 ((matrix_h-1)/2)

    typedef float matrix_type;

    matrix_type matrix[matrix_w][matrix_h] = {
#ifdef RIGHT
      {0, 0, -1, 0, 0},
      {0, -8, -21, -8, 0},
      {-1, -21, 299, -21, -1},
      {0, -8, -21, -8, 0},
      {0, 0, -1, 0, 0},
#else
      {0, -1, 0},
      {-1, 5, -1},
      {0, -1, 0}
#endif
    };

    matrix_type divisor =
#ifdef RIGHT
      179;
#else
      1;
#endif

    for (int y = matrix_h2; y < h - matrix_h2; y++)
      {
	for (int x = matrix_w2; x < w - matrix_w2; x++)
	  {
	    matrix_type sum = 0;
	    for (int x2 = 0; x2 < matrix_w; x2++)
	      {
		for (int y2 = 0; y2 < matrix_h; y2++)
		  {
		    //printf ("%d %d\n", x - matrix_w2 + x2, y - matrix_h2 + y2);
		    matrix_type v = data[x - matrix_w2 + x2, +
					 ((y - matrix_h2 + y2) * w)];
		    sum += v * matrix[x2][y2];
		  }
	      }

	    sum /= divisor;
	    unsigned char z = sum > 255 ? 255 : sum < 0 ? 0 : sum;
	    data2[x + y * w] = z;
	  }
      }

#else

#define sharp_w 3
#define sharp_h 3

    float sharpen_filter[sharp_w][sharp_h] =
      { {0, -1, 0}, {-1, 5, -1}, {0, -1, 0} };
    float sharp_sum = 1;

    for (int j = 1; j < h - 1; j++)
      {
	for (int i = 1; i < w - 1; i++)
	  {
	    float sumr = 0;

	    for (int k = 0; k < sharp_w; k++)
	      {
		for (int l = 0; l < sharp_h; l++)
		  {

		    float color = data[i - ((sharp_w - 1) >> 1) + k +
				       (j - ((sharp_h - 1) >> 1) + l) * w];
		    sumr += ((float) color) * sharpen_filter[k][l];
		  }
	      }

	    sumr /= sharp_sum;
	    int z = sumr > 255 ? 255 : sumr < 0 ? 0 : sumr;
	    data2[i + j * w] = z;
	  }
      }
#endif
  }
  data = data2;

  for (uint32 row = 0; row < h; row++)
    {
#ifdef RELEASE
      {
	// convert to 1-bit (threshold)
	unsigned char z = 0;
	unsigned char *output = data;
	unsigned char *input = data;
	for (uint32 x = 0; x < w; x++)
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
      }
#endif
      if (TIFFWriteScanline (out, data, row, 0) < 0)
	break;
      data += w * spp;
    }

  free (data2);

  TIFFClose (out);
}
