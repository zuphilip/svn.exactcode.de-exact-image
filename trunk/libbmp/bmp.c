/*
 * Stand alone BMP library.
 * Copyright (c) 2006 Rene Rebe <rene@exactcode.de>
 *
 * based on:
 *
 * Project:  libtiff tools
 * Purpose:  Convert Windows BMP files in TIFF.
 * Author:   Andrey Kiselev, dron@remotesensing.org
 *
 ******************************************************************************
 * Copyright (c) 2004, Andrey Kiselev <dron@remotesensing.org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and 
 * its documentation for any purpose is hereby granted without fee, provided
 * that (i) the above copyright notices and this permission notice appear in
 * all copies of the software and related documentation, and (ii) the names of
 * Sam Leffler and Silicon Graphics may not be used in any advertising or
 * publicity relating to the software without the specific, prior written
 * permission of Sam Leffler and Silicon Graphics.
 * 
 * THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND, 
 * EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY 
 * WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.  
 * 
 * IN NO EVENT SHALL SAM LEFFLER OR SILICON GRAPHICS BE LIABLE FOR
 * ANY SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND,
 * OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER OR NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF 
 * LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE 
 * OF THIS SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

#include <byteswap.h>

#define TIFFSwabShort(x) *x = bswap_16 (*x)
#define TIFFSwabLong(x) *x = bswap_32 (*x)

#define _TIFFmalloc malloc
#define _TIFFfree free

#include <inttypes.h>

typedef int16_t int16;
typedef uint16_t uint16;
typedef int32_t int32;
typedef uint32_t uint32;

#include "bmp.h"

#ifndef O_BINARY
# define O_BINARY 0
#endif

enum BMPType
  {
    BMPT_WIN4,      /* BMP used in Windows 3.0/NT 3.51/95 */
    BMPT_WIN5,      /* BMP used in Windows NT 4.0/98/Me/2000/XP */
    BMPT_OS21,      /* BMP used in OS/2 PM 1.x */
    BMPT_OS22       /* BMP used in OS/2 PM 2.x */
  };

/*
 * Bitmap file consists of a BMPFileHeader structure followed by a
 * BMPInfoHeader structure. An array of BMPColorEntry structures (also called
 * a colour table) follows the bitmap information header structure. The colour
 * table is followed by a second array of indexes into the colour table (the
 * actual bitmap data). Data may be comressed, for 4-bpp and 8-bpp used RLE
 * compression.
 *
 * +---------------------+
 * | BMPFileHeader       |
 * +---------------------+
 * | BMPInfoHeader       |
 * +---------------------+
 * | BMPColorEntry array |
 * +---------------------+
 * | Colour-index array  |
 * +---------------------+
 *
 * All numbers stored in Intel order with least significant byte first.
 */

enum BMPComprMethod
  {
    BMPC_RGB = 0L,          /* Uncompressed */
    BMPC_RLE8 = 1L,         /* RLE for 8 bpp images */
    BMPC_RLE4 = 2L,         /* RLE for 4 bpp images */
    BMPC_BITFIELDS = 3L,    /* Bitmap is not compressed and the colour table
			     * consists of three DWORD color masks that specify
			     * the red, green, and blue components of each
			     * pixel. This is valid when used with
			     * 16- and 32-bpp bitmaps. */
    BMPC_JPEG = 4L,         /* Indicates that the image is a JPEG image. */
    BMPC_PNG = 5L           /* Indicates that the image is a PNG image. */
  };

enum BMPLCSType                 /* Type of logical color space. */
  {
    BMPLT_CALIBRATED_RGB = 0,	/* This value indicates that endpoints and
				 * gamma values are given in the appropriate
				 * fields. */
    BMPLT_DEVICE_RGB = 1,
    BMPLT_DEVICE_CMYK = 2
  };

typedef struct
{
  int32   iCIEX;
  int32   iCIEY;
  int32   iCIEZ;
} BMPCIEXYZ;

typedef struct                  /* This structure contains the x, y, and z */
{				/* coordinates of the three colors that */
				/* correspond */
  BMPCIEXYZ   iCIERed;        /* to the red, green, and blue endpoints for */
  BMPCIEXYZ   iCIEGreen;      /* a specified logical color space. */
  BMPCIEXYZ	iCIEBlue;
} BMPCIEXYZTriple;

typedef struct
{
  char	bType[2];       /* Signature "BM" */
  uint32	iSize;          /* Size in bytes of the bitmap file. Should
				 * always be ignored while reading because
				 * of error in Windows 3.0 SDK's description
				 * of this field */
  uint16	iReserved1;     /* Reserved, set as 0 */
  uint16	iReserved2;     /* Reserved, set as 0 */
  uint32	iOffBits;       /* Offset of the image from file start in bytes */
} BMPFileHeader;

/* File header size in bytes: */
const int       BFH_SIZE = 14;

typedef struct
{
  uint32	iSize;          /* Size of BMPInfoHeader structure in bytes.
				 * Should be used to determine start of the
				 * colour table */
  int32	iWidth;         /* Image width */
  int32	iHeight;        /* Image height. If positive, image has bottom
			 * left origin, if negative --- top left. */
  int16	iPlanes;        /* Number of image planes (must be set to 1) */
  int16	iBitCount;      /* Number of bits per pixel (1, 4, 8, 16, 24
			 * or 32). If 0 then the number of bits per
			 * pixel is specified or is implied by the
			 * JPEG or PNG format. */
  uint32	iCompression;	/* Compression method */
  uint32	iSizeImage;     /* Size of uncomressed image in bytes. May
				 * be 0 for BMPC_RGB bitmaps. If iCompression
				 * is BI_JPEG or BI_PNG, iSizeImage indicates
				 * the size of the JPEG or PNG image buffer. */
  int32	iXPelsPerMeter; /* X resolution, pixels per meter (0 if not used) */
  int32	iYPelsPerMeter; /* Y resolution, pixels per meter (0 if not used) */
  uint32	iClrUsed;       /* Size of colour table. If 0, iBitCount should
				 * be used to calculate this value
				 * (1<<iBitCount). This value should be
				 * unsigned for proper shifting. */
  int32	iClrImportant;  /* Number of important colours. If 0, all
			 * colours are required */

  /*
   * Fields above should be used for bitmaps, compatible with Windows NT 3.51
   * and earlier. Windows 98/Me, Windows 2000/XP introduces additional fields:
   */

  int32	iRedMask;       /* Colour mask that specifies the red component
			 * of each pixel, valid only if iCompression
			 * is set to BI_BITFIELDS. */
  int32	iGreenMask;     /* The same for green component */
  int32	iBlueMask;      /* The same for blue component */
  int32	iAlphaMask;     /* Colour mask that specifies the alpha
			 * component of each pixel. */
  uint32	iCSType;        /* Colour space of the DIB. */
  BMPCIEXYZTriple sEndpoints; /* This member is ignored unless the iCSType
			       * member specifies BMPLT_CALIBRATED_RGB. */
  int32	iGammaRed;      /* Toned response curve for red. This member
			 * is ignored unless color values are
			 * calibrated RGB values and iCSType is set to
			 * BMPLT_CALIBRATED_RGB. Specified
			 * in 16^16 format. */
  int32	iGammaGreen;    /* Toned response curve for green. */
  int32	iGammaBlue;     /* Toned response curve for blue. */
} BMPInfoHeader;

/*
 * Info header size in bytes:
 */
const unsigned int  BIH_WIN4SIZE = 40; /* for BMPT_WIN4 */
const unsigned int  BIH_WIN5SIZE = 57; /* for BMPT_WIN5 */
const unsigned int  BIH_OS21SIZE = 12; /* for BMPT_OS21 */
const unsigned int  BIH_OS22SIZE = 64; /* for BMPT_OS22 */

/*
 * We will use plain byte array instead of this structure, but declaration
 * provided for reference
 */
typedef struct
{
  char       bBlue;
  char       bGreen;
  char       bRed;
  char       bReserved;      /* Must be 0 */
} BMPColorEntry;

/*
 * Image data in BMP file stored in BGR (or ABGR) format. We should rearrange
 * pixels to RGB (RGBA) format.
 */
static void
rearrangePixels(unsigned char* buf, uint32 width, uint32 bit_count)
{
  char tmp;
  uint32 i;
  
  switch(bit_count) {
    
  case 16:    /* FIXME: need a sample file */
    break;
    
  case 24:
    for (i = 0; i < width; i++, buf += 3) {
      tmp = *buf;
      *buf = *(buf + 2);
      *(buf + 2) = tmp;
    }
    break;
  
  case 32:
    {
      unsigned char* buf1 = buf;
      for (i = 0; i < width; i++, buf += 4) {
	tmp = *buf;
	*buf1++ = *(buf + 2);
	*buf1++ = *(buf + 1);
	*buf1++ = tmp;
      }
    }
    break;
    
  default:
    break;
  }
}

unsigned char* read_bmp (FILE* fd, int* w, int* h, int* bps, int* spp,
			 int* xres, int* yres,
			 unsigned char** color_table, int* color_table_size,
			 int* color_table_elements)
{
  BMPFileHeader file_hdr;
  BMPInfoHeader info_hdr;
  enum BMPType bmp_type;
  
  uint32  clr_tbl_size, n_clr_elems = 3;
  unsigned char *clr_tbl;
  
  uint32	row, stride;

  unsigned char* data = 0;
  
  fread (file_hdr.bType, 1, 2, fd);
  if(file_hdr.bType[0] != 'B' || file_hdr.bType[1] != 'M') {
    fprintf(stderr, "File is not a BMP\n");
    goto bad;
  }
  
  /* -------------------------------------------------------------------- */
  /*      Read the BMPFileHeader. We need iOffBits value only             */
  /* -------------------------------------------------------------------- */
  fseek(fd, 10, SEEK_SET);
  fread(&file_hdr.iOffBits, 1, 4, fd);
#ifdef __BIG_ENDIAN__
  TIFFSwabLong(&file_hdr.iOffBits);
#endif

  fseek(fd, 0, SEEK_END);
  file_hdr.iSize = ftell(fd);
  
  /* -------------------------------------------------------------------- */
  /*      Read the BMPInfoHeader.                                         */
  /* -------------------------------------------------------------------- */
  
  fseek(fd, BFH_SIZE, SEEK_SET);
  fread(&info_hdr.iSize, 1, 4, fd);
#ifdef __BIG_ENDIAN__
  TIFFSwabLong(&info_hdr.iSize);
#endif
  
  if (info_hdr.iSize == BIH_WIN4SIZE)
    bmp_type = BMPT_WIN4;
  else if (info_hdr.iSizey == BIH_OS21SIZE)
    bmp_type = BMPT_OS21;
  else if (info_hdr.iSize == BIH_OS22SIZE || info_hdr.iSize == 16)
    bmp_type = BMPT_OS22;
  else
    bmp_type = BMPT_WIN5;
  
  if (bmp_type == BMPT_WIN4 || bmp_type == BMPT_WIN5 || bmp_type == BMPT_OS22) {
    fread(&info_hdr.iWidth, 1, 4, fd);
    fread(&info_hdr.iHeight, 1, 4, fd);
    fread(&info_hdr.iPlanes, 1, 2, fd);
    fread(&info_hdr.iBitCount, 1, 2, fd);
    fread(&info_hdr.iCompression, 1, 4, fd);
    fread(&info_hdr.iSizeImage, 1, 4, fd);
    fread(&info_hdr.iXPelsPerMeter, 1, 4, fd);
    fread(&info_hdr.iYPelsPerMeter, 1, 4, fd);
    fread(&info_hdr.iClrUsed, 1, 4, fd);
    fread(&info_hdr.iClrImportant, 1, 4, fd);
    fread(&info_hdr.iRedMask, 1, 4, fd);
    fread(&info_hdr.iGreenMask, 1, 4, fd);
    fread(&info_hdr.iBlueMask, 1, 4, fd);
    fread(&info_hdr.iAlphaMask, 1, 4, fd);
#ifdef __BIG_ENDIAN__
    TIFFSwabLong(&info_hdr.iWidth);
    TIFFSwabLong(&info_hdr.iHeight);
    TIFFSwabShort(&info_hdr.iPlanes);
    TIFFSwabShort(&info_hdr.iBitCount);
    TIFFSwabLong(&info_hdr.iCompression);
    TIFFSwabLong(&info_hdr.iSizeImage);
    TIFFSwabLong(&info_hdr.iXPelsPerMeter);
    TIFFSwabLong(&info_hdr.iYPelsPerMeter);
    TIFFSwabLong(&info_hdr.iClrUsed);
    TIFFSwabLong(&info_hdr.iClrImportant);
    TIFFSwabLong(&info_hdr.iRedMask);
    TIFFSwabLong(&info_hdr.iGreenMask);
    TIFFSwabLong(&info_hdr.iBlueMask);
    TIFFSwabLong(&info_hdr.iAlphaMask);
#endif
    n_clr_elems = 4;
    *xres = ((double)info_hdr.iXPelsPerMeter * 2.54 + 0.05) / 100;
    *yres = ((double)info_hdr.iYPelsPerMeter * 2.54 + 0.05) / 100;
  }
  
  if (bmp_type == BMPT_OS22) {
    /* 
     * FIXME: different info in different documents
     * regarding this!
     */
    n_clr_elems = 3;
  }

  if (bmp_type == BMPT_OS21) {
    int16  iShort;
    
    fread(&iShort, 1, 2, fd);
#ifdef __BIG_ENDIAN__
    TIFFSwabShort(&iShort);
#endif
    info_hdr.iWidth = iShort;
    fread(&iShort, 1, 2, fd);
#ifdef __BIG_ENDIAN__
    TIFFSwabShort(&iShort);
#endif
    info_hdr.iHeight = iShort;
    fread(&iShort, 1, 2, fd);
#ifdef __BIG_ENDIAN__
    TIFFSwabShort(&iShort);
#endif
    info_hdr.iPlanes = iShort;
    fread(&iShort, 1, 2, fd);
#ifdef __BIG_ENDIAN__
    TIFFSwabShort(&iShort);
#endif
    info_hdr.iBitCount = iShort;
    info_hdr.iCompression = BMPC_RGB;
    n_clr_elems = 3;
  }
  
  if (info_hdr.iBitCount != 1  && info_hdr.iBitCount != 4  &&
      info_hdr.iBitCount != 8  && info_hdr.iBitCount != 16 &&
      info_hdr.iBitCount != 24 && info_hdr.iBitCount != 32) {
    fprintf(stderr, "Cannot process BMP file with bit count %d\n",
	    info_hdr.iBitCount);
    return 0;
  }
  
  *w = info_hdr.iWidth;
  *h = (info_hdr.iHeight > 0) ? info_hdr.iHeight : -info_hdr.iHeight;
  
  switch (info_hdr.iBitCount)
    {
    case 1:
    case 4:
    case 8:
      *spp = 1;
      *bps = info_hdr.iBitCount;

      /* Allocate memory for colour table and read it. */
      if (info_hdr.iClrUsed)
	clr_tbl_size = ((uint32)(1 << *bps) < info_hdr.iClrUsed) ?
	  1 << *bps : info_hdr.iClrUsed;
      else
	clr_tbl_size = 1 << *bps;
      clr_tbl = (unsigned char *)
	_TIFFmalloc(n_clr_elems * clr_tbl_size);
      if (!clr_tbl) {
	fprintf(stderr, "Can't allocate space for color table\n");
	goto bad;
      }
      
      /*printf ("n_clr_elems: %d, clr_tbl_size: %d\n",
	n_clr_elems, clr_tbl_size); */
      
      fseek(fd, BFH_SIZE + info_hdr.iSize, SEEK_SET);
      fread(clr_tbl, 1, n_clr_elems * clr_tbl_size, fd);
      
      /*for(clr = 0; clr < clr_tbl_size; ++clr) {
	printf ("%d: r: %d g: %d b: %d\n",
		clr,
		clr_tbl[clr*n_clr_elems+2],
		clr_tbl[clr*n_clr_elems+1],
		clr_tbl[clr*n_clr_elems]);
      }*/
      break;
    
    case 16:
    case 24:
      *spp = 3;
      *bps = info_hdr.iBitCount / *spp;
      break;
    
    case 32:
      *spp = 3;
      *bps = 8;
      break;
    
    default:
      break;
    }
  
  stride = (*w * *spp * *bps + 7) / 8;
  /*printf ("w: %d, h: %d, spp: %d, bps: %d, colorspace: %d\n",
   *w, *h, *spp, *bps, info_hdr.iCompression); */
  
  // detect old style bitmask images
  if (info_hdr.iCompression == BMPC_RGB && info_hdr.iBitCount == 16)
    {
      /*printf ("implicit non-RGB image\n"); */
      info_hdr.iCompression = BMPC_BITFIELDS;
      info_hdr.iBlueMask = 0x1f;
      info_hdr.iGreenMask = 0x1f << 5;
      info_hdr.iRedMask = 0x1f << 10;
    }
  
  /* -------------------------------------------------------------------- */
  /*  Read uncompressed image data.                                       */
  /* -------------------------------------------------------------------- */

  switch (info_hdr.iCompression) {
  case BMPC_BITFIELDS:
    // we convert those to RGB for easier use
    *bps = 8;
    stride = (*w * *spp * *bps + 7) / 8;
  case BMPC_RGB:
    {
      uint32 file_stride = ((*w * info_hdr.iBitCount + 7) / 8 + 3) / 4 * 4;
      
      /*printf ("bitcount: %d, stride: %d, file stride: %d\n",
	      info_hdr.iBitCount, stride, file_stride);
      
      printf ("red mask: %x, green mask: %x, blue mask: %x\n",
      info_hdr.iRedMask, info_hdr.iGreenMask, info_hdr.iBlueMask); */
      
      data = _TIFFmalloc (stride * *h);
      
      if (!data) {
	fprintf(stderr, "Can't allocate space for image buffer\n");
	goto bad1;
      }
      
      for (row = 0; row < *h; row++) {
	uint32 offset;
	
	if (info_hdr.iHeight > 0)
	  offset = file_hdr.iOffBits + (*h - row - 1) * file_stride;
	else
	  offset = file_hdr.iOffBits + row * file_stride;
	
	if (fseek(fd, offset, SEEK_SET) == (off_t)-1) {
	  fprintf(stderr, "scanline %lu: Seek error\n", (unsigned long) row);
	}
	
	if (fread(data + stride*row, 1, stride, fd) < 0) {
	  fprintf(stderr, "scanline %lu: Read error\n",
		  (unsigned long) row);
	}
	
	// convert to RGB
	if (info_hdr.iCompression == BMPC_BITFIELDS)
	  {
	    int last_bit_set (int v)
	    {
	      unsigned int i;
	      for (i = sizeof (int) * 8 - 1; i > 0; --i) {
		if (v & (1L << i))
		  return i;
	      }
	      return 0;
	    }
	    
	    unsigned char* row_ptr = data + stride*row;
	    unsigned char* r16_ptr = row_ptr + file_stride - 2;
	    unsigned char* rgb_ptr = row_ptr + stride - 3;
	    
	    int r_shift = last_bit_set (info_hdr.iRedMask) - 7;
	    int g_shift = last_bit_set (info_hdr.iGreenMask) - 7;
	    int b_shift = last_bit_set (info_hdr.iBlueMask) - 7;
	    
	    for ( ; rgb_ptr >= row_ptr; r16_ptr -= 2, rgb_ptr -= 3)
	      {
		int val = (r16_ptr[0] << 0) + (r16_ptr[1] << 8);
		if (r_shift > 0)
		  rgb_ptr[0] = (val & info_hdr.iRedMask) >> r_shift;
		else
		  rgb_ptr[0] = (val & info_hdr.iRedMask) << -r_shift;
		if (g_shift > 0)
		  rgb_ptr[1] = (val & info_hdr.iGreenMask) >> g_shift;
		else
		  rgb_ptr[1] = (val & info_hdr.iGreenMask) << -g_shift;
		if (b_shift > 0)
		  rgb_ptr[2] = (val & info_hdr.iBlueMask) >> b_shift;
		else
		  rgb_ptr[2] = (val & info_hdr.iBlueMask) << -b_shift;
	      }
	  }
	else
	  rearrangePixels(data + stride*row, *w, info_hdr.iBitCount);
      }
    }
    break;
    
    /* -------------------------------------------------------------------- */
    /*  Read compressed image data.                                         */
    /* -------------------------------------------------------------------- */
  case BMPC_RLE4:
  case BMPC_RLE8:
    {
      uint32		i, j, k, runlength, x;
      uint32		compr_size, uncompr_size;
      unsigned char   *comprbuf;
      unsigned char   *uncomprbuf;

      printf ("RLE%s compressed\n", info_hdr.iCompression == BMPC_RLE4 ? "4" : "8");
      
      compr_size = file_hdr.iSize - file_hdr.iOffBits;
      uncompr_size = *w * *h;
      
      comprbuf = (unsigned char *) _TIFFmalloc( compr_size );
      if (!comprbuf) {
	fprintf (stderr, "Can't allocate space for compressed scanline buffer\n");
	goto bad1;
      }
      uncomprbuf = (unsigned char *) _TIFFmalloc( uncompr_size );
      if (!uncomprbuf) {
	fprintf (stderr, "Can't allocate space for uncompressed scanline buffer\n");
	goto bad1;
      }
      
      fseek(fd, file_hdr.iOffBits, SEEK_SET);
      fread(comprbuf, 1, compr_size, fd);
      i = j = x = 0;
      
      while( j < uncompr_size && i < compr_size ) {
	if ( comprbuf[i] ) {
	  runlength = comprbuf[i++];
	  for ( k = 0;
		runlength > 0 && j < uncompr_size && i < compr_size && x < *w;
		++k, ++x) {
	    if (info_hdr.iBitCount == 8)
	      uncomprbuf[j++] = comprbuf[i];
	    else {
	      if ( k & 0x01 )
		uncomprbuf[j++] = comprbuf[i] & 0x0F;
	      else
		uncomprbuf[j++] = (comprbuf[i] & 0xF0) >> 4;
	    }
	    runlength--;
	  }
	  i++;
	} else {
	  i++;
	  if ( comprbuf[i] == 0 ) {         /* Next scanline */
	    i++;
	    x = 0;;
	  }
	  else if ( comprbuf[i] == 1 )    /* End of image */
	    break;
	  else if ( comprbuf[i] == 2 ) {  /* Move to... */
	    i++;
	    if ( i < compr_size - 1 ) {
	      j += comprbuf[i] + comprbuf[i+1] * *w;
	      i += 2;
	    }
	    else
	      break;
	  } else {                         /* Absolute mode */
	    runlength = comprbuf[i++];
	    for ( k = 0; k < runlength && j < uncompr_size && i < compr_size; k++, x++)
	      {
		if (info_hdr.iBitCount == 8)
		  uncomprbuf[j++] = comprbuf[i++];
		else {
		  if ( k & 0x01 )
		    uncomprbuf[j++] = comprbuf[i++] & 0x0F;
		  else
		    uncomprbuf[j++] = (comprbuf[i] & 0xF0) >> 4;
		}
	      }
	    /* word boundary alignment */
	    if (info_hdr.iBitCount == 4)
	      k /= 2;
	    if ( k & 0x01 )
	      i++;
	  }
	}
      }
      
      _TIFFfree(comprbuf);
      data = (unsigned char *) _TIFFmalloc( uncompr_size );
      if (!data) {
	fprintf (stderr, "Can't allocate space for final uncompressed scanline buffer\n");
	goto bad1;
      }
      
      // TODO: suboptimal, later improve the above to yield the corrent orientation natively
      for (row = 0; row < *h; ++row) {
	memcpy (data + row * *w, uncomprbuf + (*h - 1 - row) * *w, *w);
	rearrangePixels(data + row * *w, *w, info_hdr.iBitCount);
      }
      
      _TIFFfree(uncomprbuf);
      *bps = 8;
    }
    break;
  } /* switch */
  
  /* export the table */
  *color_table = clr_tbl;
  *color_table_size = clr_tbl_size;
  *color_table_elements = n_clr_elems;
  goto bad;
  
 bad1:
  if (clr_tbl)
    _TIFFfree(clr_tbl);
  clr_tbl = NULL;
  
 bad:

  return data;
}
