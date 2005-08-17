
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

int main (int argc, char* argv[])
{
        TIFF *out;
	char thing [1024];
        unsigned char *inbuf, *outbuf;

	int w, h, bpp, spp;
	w = h = 16;
	bpp = spp = 1;

        uint32 rowsperstrip = (uint32) -1;

	uint16 compression = COMPRESSION_PACKBITS;

        out = TIFFOpen("file.tiff", "w");
        if (out == NULL)
                return (-1);
        TIFFSetField(out, TIFFTAG_IMAGEWIDTH, w);
        TIFFSetField(out, TIFFTAG_IMAGELENGTH, h);
        TIFFSetField(out, TIFFTAG_BITSPERSAMPLE, bpp);
        TIFFSetField(out, TIFFTAG_SAMPLESPERPIXEL, spp);
        TIFFSetField(out, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);

        TIFFSetField(out, TIFFTAG_COMPRESSION, compression);
        TIFFSetField(out, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
        sprintf(thing, "B&W version of %s", "file.tiff");
        TIFFSetField(out, TIFFTAG_IMAGEDESCRIPTION, thing);
        TIFFSetField(out, TIFFTAG_SOFTWARE, "tiff2bw");
        outbuf = (unsigned char *)_TIFFmalloc(TIFFScanlineSize(out));
        TIFFSetField(out, TIFFTAG_ROWSPERSTRIP,
        TIFFDefaultStripSize(out, rowsperstrip));

	for (uint32 row = 0; row < h; row++) {
                if (TIFFWriteScanline(out, outbuf, row, 0) < 0)
                       break;
        }

        TIFFClose(out);
}
