/*
 * C++ BMP library.
 * Copyright (c) 2006 - 2007 Rene Rebe <rene@exactcode.de>
 *
 * loosely based on (in the past more so, but more and more parts got rewritten):
 *
 * Project:  libtiff tools
 * Purpose:  Convert Windows BMP files in TIFF.
 * Author:   Andrey Kiselev, dron@remotesensing.org
 *
 */

#include <iostream>

#include "bmp.hh"

#include "Colorspace.hh"

#include <string.h>
#include <ctype.h>

#ifdef __FreeBSD__
#include <machine/endian.h>
#else
#include <endian.h>
#endif

#include <byteswap.h>

#define TIFFSwabShort(x) *x = bswap_16 (*x)
#define TIFFSwabLong(x) *x = bswap_32 (*x)

#include <inttypes.h>

typedef int16_t int16;
typedef uint16_t uint16;
typedef int32_t int32;
typedef uint32_t uint32;

#define _TIFFmalloc malloc
#define _TIFFfree free

static int last_bit_set (int v)
{
  unsigned int i;
  for (i = sizeof (int) * 8 - 1; i > 0; --i) {
    if (v & (1L << i))
      return i;
  }
  return 0;
}

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
  BMPCIEXYZ   iCIEBlue;
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
}  __attribute__((packed)) BMPFileHeader;

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
}  __attribute__((packed)) BMPInfoHeader;

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
 * Image data in BMP file stored in BGR (or ABGR) format. We rearrange
 * pixels to RGB (RGBA) format.
 */
static void
rearrangePixels(uint8_t* buf, uint32 width, uint32 bit_count)
{
  char tmp;
  
  switch (bit_count) {
    
  case 16:    /* FIXME: need a sample file */
    break;
    
  case 24:
    for (int i = 0; i < width; i++, buf += 3) {
      tmp = *buf;
      *buf = *(buf + 2);
      *(buf + 2) = tmp;
    }
    break;
  
  case 32:
    {
      uint8_t* buf1 = buf;
      for (int i = 0; i < width; i++, buf += 4) {
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

bool BMPCodec::readImage (std::istream* stream, Image& image)
{
  BMPFileHeader file_hdr;
  BMPInfoHeader info_hdr;
  enum BMPType bmp_type;

  uint32 row, stride;
  
  uint32  clr_tbl_size = 0, n_clr_elems = 3;
  uint8_t *clr_tbl = 0;
  uint8_t* data = 0;
  
  stream->read ((char*)&file_hdr.bType, 2);
  if(file_hdr.bType[0] != 'B' || file_hdr.bType[1] != 'M') {
    stream->seekg (0);
    return false;
  }
  
  /* -------------------------------------------------------------------- */
  /*      Read the BMPFileHeader. We need iOffBits value only             */
  /* -------------------------------------------------------------------- */
  stream->seekg (10);
  stream->read ((char*)&file_hdr.iOffBits, 4);
#if __BYTE_ORDER == __BIG_ENDIAN
  TIFFSwabLong(&file_hdr.iOffBits);
#endif

  // fix the iSize, in early BMP file this is pure garbage
  stream->seekg (0, std::ios::end);
  file_hdr.iSize = stream->tellg ();
  
  /* -------------------------------------------------------------------- */
  /*      Read the BMPInfoHeader.                                         */
  /* -------------------------------------------------------------------- */
  
  stream->seekg (BFH_SIZE);
  stream->read ((char*)&info_hdr.iSize, 4);
#if __BYTE_ORDER == __BIG_ENDIAN
  TIFFSwabLong(&info_hdr.iSize);
#endif
  
  if (info_hdr.iSize == BIH_WIN4SIZE)
    bmp_type = BMPT_WIN4;
  else if (info_hdr.iSize == BIH_OS21SIZE)
    bmp_type = BMPT_OS21;
  else if (info_hdr.iSize == BIH_OS22SIZE || info_hdr.iSize == 16)
    bmp_type = BMPT_OS22;
  else
    bmp_type = BMPT_WIN5;
  
  if (bmp_type == BMPT_WIN4 || bmp_type == BMPT_WIN5 ||
      bmp_type == BMPT_OS22) {
    stream->read((char*)&info_hdr.iWidth, 4);
    stream->read((char*)&info_hdr.iHeight, 4);
    stream->read((char*)&info_hdr.iPlanes, 2);
    stream->read((char*)&info_hdr.iBitCount, 2);
    stream->read((char*)&info_hdr.iCompression, 4);
    stream->read((char*)&info_hdr.iSizeImage, 4);
    stream->read((char*)&info_hdr.iXPelsPerMeter, 4);
    stream->read((char*)&info_hdr.iYPelsPerMeter, 4);
    stream->read((char*)&info_hdr.iClrUsed, 4);
    stream->read((char*)&info_hdr.iClrImportant, 4);
    stream->read((char*)&info_hdr.iRedMask, 4);
    stream->read((char*)&info_hdr.iGreenMask, 4);
    stream->read((char*)&info_hdr.iBlueMask, 4);
    stream->read((char*)&info_hdr.iAlphaMask, 4);
#if __BYTE_ORDER == __BIG_ENDIAN
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
    image.xres = (int) ((2.54 * info_hdr.iXPelsPerMeter + .05) / 100);
    image.yres = (int) ((2.54 * info_hdr.iYPelsPerMeter + .05) / 100);
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
    
    stream->read ((char*)&iShort, 2);
#if __BYTE_ORDER == __BIG_ENDIAN
    TIFFSwabShort(&iShort);
#endif
    info_hdr.iWidth = iShort;
    stream->read ((char*)&iShort, 2);
#if __BYTE_ORDER == __BIG_ENDIAN
    TIFFSwabShort(&iShort);
#endif
    info_hdr.iHeight = iShort;
    stream->read ((char*)&iShort, 2);
#if __BYTE_ORDER == __BIG_ENDIAN
    TIFFSwabShort(&iShort);
#endif
    info_hdr.iPlanes = iShort;
    stream->read ((char*)&iShort, 2);
#if __BYTE_ORDER == __BIG_ENDIAN
    TIFFSwabShort(&iShort);
#endif
    info_hdr.iBitCount = iShort;
    info_hdr.iCompression = BMPC_RGB;
    n_clr_elems = 3;
  }
  
  if (info_hdr.iBitCount != 1  && info_hdr.iBitCount != 4  &&
      info_hdr.iBitCount != 8  && info_hdr.iBitCount != 16 &&
      info_hdr.iBitCount != 24 && info_hdr.iBitCount != 32) {
    std::cerr << "Cannot process BMP file with bit count " << info_hdr.iBitCount << "\n";
    return 0;
  }
  
  image.w = info_hdr.iWidth;
  image.h = (info_hdr.iHeight > 0) ? info_hdr.iHeight : -info_hdr.iHeight;
  
  switch (info_hdr.iBitCount)
    {
    case 1:
    case 4:
    case 8:
      image.spp = 1;
      image.bps = info_hdr.iBitCount;

      /* Allocate memory for colour table and read it. */
      if (info_hdr.iClrUsed)
	clr_tbl_size = ((uint32)(1 << image.bps) < info_hdr.iClrUsed) ?
	  1 << image.bps : info_hdr.iClrUsed;
      else
	clr_tbl_size = 1 << image.bps;
      clr_tbl = (uint8_t *)
	_TIFFmalloc(n_clr_elems * clr_tbl_size);
      if (!clr_tbl) {
	std::cerr << "Can't allocate space for color table\n" << std::endl;
	goto bad;
      }
      
      /*printf ("n_clr_elems: %d, clr_tbl_size: %d\n",
	n_clr_elems, clr_tbl_size); */
      
      stream->seekg (BFH_SIZE + info_hdr.iSize);
      stream->read ((char*)clr_tbl, n_clr_elems * clr_tbl_size);
      
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
      image.spp = 3;
      image.bps = info_hdr.iBitCount / image.spp;
      break;
    
    case 32:
      image.spp = 3;
      image.bps = 8;
      break;
    
    default:
      break;
    }
  
  stride = image.Stride ();
  /*printf ("w: %d, h: %d, spp: %d, bps: %d, colorspace: %d\n",
   *w, *h, *spp, *bps, info_hdr.iCompression); */
  
  // detect old style bitmask images
  if (info_hdr.iCompression == BMPC_RGB && info_hdr.iBitCount == 16)
    {
      //std::cerr << "implicit non-RGB image\n";
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
    // we unpack bitfields to plain RGB
    image.bps = 8;
    stride = image.Stride ();
    
  case BMPC_RGB:
    {
      uint32 file_stride = ((image.w * info_hdr.iBitCount + 7) / 8 + 3) / 4 * 4;
      
      /*printf ("bitcount: %d, stride: %d, file stride: %d\n",
	      info_hdr.iBitCount, stride, file_stride);
     
      printf ("red mask: %x, green mask: %x, blue mask: %x\n",
      info_hdr.iRedMask, info_hdr.iGreenMask, info_hdr.iBlueMask); */
      
      data = (uint8_t*) _TIFFmalloc (stride*image.h);
      uint8_t* row_data = (uint8_t*) _TIFFmalloc (file_stride);
      if (!data || !row_data) {
	std::cerr << "Can't allocate space for image buffer\n";
	goto bad1;
      }
      
      const int r_shift = last_bit_set (info_hdr.iRedMask) - 7;
      const int g_shift = last_bit_set (info_hdr.iGreenMask) - 7;
      const int b_shift = last_bit_set (info_hdr.iBlueMask) - 7;
      
      for (row = 0; row < image.h; ++row) {
	std::istream::pos_type offset;
	
	if (info_hdr.iHeight > 0)
	  offset = file_hdr.iOffBits + (image.h - row - 1) * file_stride;
	else
	  offset = file_hdr.iOffBits + row * file_stride;
	
	stream->seekg (offset);
	/*
	  if (stream->tellg () != offset) {
	  fprintf(stderr, "scanline %lu: Seek error\n", (unsigned long) row);
	  }
	*/
	
	if (stream->read ((char*)row_data, file_stride) < 0) {
	  std::cerr << "scanline " << row << ": Read error\n";
	}
	
	// convert to RGB
	if (info_hdr.iCompression == BMPC_BITFIELDS)
	  {
	    uint8_t* bf_ptr = row_data;
	    uint8_t* rgb_ptr = data + stride * row;
	    
	    for (int i = 0; i < image.w; ++i, rgb_ptr += 3)
	      {
		int val = 0;
		for (int bits = 0; bits < info_hdr.iBitCount; bits += 8)
		  val |= (*bf_ptr++) << bits;
		    
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
	else {
	  rearrangePixels (row_data, image.w, info_hdr.iBitCount);
	  memcpy (data+stride*row, row_data, stride);
	}
      }
      _TIFFfree(row_data);
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
      uint8_t   *comprbuf;
      uint8_t   *uncomprbuf;
      
      //std::cerr << "RLE" << (info_hdr.iCompression == BMPC_RLE4 ? "4" : "8")
      //	<< " compressed\n";
      
      compr_size = file_hdr.iSize - file_hdr.iOffBits;
      uncompr_size = image.w * image.h;
      
      comprbuf = (uint8_t *) _TIFFmalloc( compr_size );
      if (!comprbuf) {
	std::cerr << "Can't allocate space for compressed scanline buffer\n";
	goto bad1;
      }
      uncomprbuf = (uint8_t *) _TIFFmalloc( uncompr_size );
      if (!uncomprbuf) {
	std::cerr, "Can't allocate space for uncompressed scanline buffer\n";
	goto bad1;
      }
      
      stream->seekg (file_hdr.iOffBits);
      stream->read ((char*)comprbuf, compr_size);
      i = j = x = 0;
      
      while( j < uncompr_size && i < compr_size ) {
	if ( comprbuf[i] ) {
	  runlength = comprbuf[i++];
	  for ( k = 0;
		runlength > 0 && j < uncompr_size && i < compr_size && x < image.w;
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
	      j += comprbuf[i] + comprbuf[i+1] * image.w;
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
      data = (uint8_t *) _TIFFmalloc( uncompr_size );
      if (!data) {
	std::cerr << "Can't allocate space for final uncompressed scanline buffer\n";
	goto bad1;
      }
      
      // TODO: suboptimal, later improve the above to yield the corrent orientation natively
      for (row = 0; row < image.h; ++row) {
	memcpy (data + row * image.w, uncomprbuf + (image.h - 1 - row) * image.w, image.w);
	rearrangePixels(data + row * image.w, image.w, info_hdr.iBitCount);
      }
      
      _TIFFfree(uncomprbuf);
      image.bps = 8;
    }
    break;
  } /* switch */
  
  image.setRawData (data);
  
  // convert to RGB color-space - we do not handle palete images internally
  
  // no color table anyway or RGB* ?
  if (clr_tbl && image.spp < 3)
    {
      uint16_t* rmap = new uint16_t [1 << image.bps];
      uint16_t* gmap = new uint16_t [1 << image.bps];
      uint16_t* bmap = new uint16_t [1 << image.bps];
      
      for (int i = 0; i < (1 << image.bps); ++i) {
	// BMP maps have BGR order ...
	rmap[i] = clr_tbl [i * n_clr_elems + 2] << 8;
	gmap[i] = clr_tbl [i * n_clr_elems + 1] << 8;
	bmap[i] = clr_tbl [i * n_clr_elems + 0] << 8;
      }
      
      colorspace_de_palette (image, clr_tbl_size, rmap, gmap, bmap);
      
      delete (rmap);
      delete (gmap);
      delete (bmap);
      
      _TIFFfree(clr_tbl);
      clr_tbl = NULL;
    }
  
  return true;
  
 bad1:
  if (clr_tbl)
    _TIFFfree(clr_tbl);
  clr_tbl = NULL;
  
  return false;
  
 bad:
  return false;
}

bool BMPCodec::writeImage (std::ostream* stream, Image& image, int quality,
			   const std::string& compress)
{
  if (image.bps != 8 || image.spp != 3) {
    std::cerr << "only writing 24bit BMP is supported right now" << std::endl;
    return false;
  }
  
  BMPFileHeader file_hdr;
  BMPInfoHeader info_hdr;
  enum BMPType bmp_type;

  int stride = image.Stride ();
  
  uint32  clr_tbl_size = 0, n_clr_elems = 3;
  uint8_t *clr_tbl = 0;
  
  memset (&file_hdr, 0, sizeof (file_hdr));
  memset (&info_hdr, 0, sizeof (info_hdr));

  // BMPFileHeader

  file_hdr.bType[0] = 'B';
  file_hdr.bType[1] = 'M';
  
  // BMPInfoHeader
  
  info_hdr.iSize = BIH_WIN5SIZE;
  info_hdr.iWidth = image.w;
  info_hdr.iHeight = image.h;
  info_hdr.iPlanes = 1;
  info_hdr.iBitCount = image.spp * image.bps;
  info_hdr.iCompression = BMPC_RGB;
  info_hdr.iSizeImage  = image.Stride()*image.h; // TODO: compressed size
  info_hdr.iXPelsPerMeter = (int32) (image.xres * 100 / 2.54);
  info_hdr.iYPelsPerMeter = (int32) (image.yres * 100 / 2.54);
  info_hdr.iClrUsed = 0; // TODO
  info_hdr.iClrImportant = 0; // TODO
  info_hdr.iRedMask = 0;
  info_hdr.iGreenMask = 0; // TODO
  info_hdr.iBlueMask = 0; // TODO
  info_hdr.iAlphaMask = 0; // TODO

  // BMP image payload needs to be 4 byte alligned :-(
  int file_stride = ((image.w * info_hdr.iBitCount + 7) / 8 + 3) / 4 * 4;
  
  file_hdr.iOffBits = BFH_SIZE + BIH_WIN5SIZE;
  file_hdr.iSize =  file_hdr.iOffBits + file_stride * image.h;
  
  // swab non byte fields
#if __BYTE_ORDER == __BIG_ENDIAN
  TIFFSwabLong(&file_hdr.iSize);
  TIFFSwabLong(&file_hdr.iOffBits);
  
  TIFFSwabLong(&info_hdr.iSize);
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

  // write header meta info
  stream->write ((char*)&file_hdr, BFH_SIZE);
  stream->write ((char*)&info_hdr, BIH_WIN5SIZE);
  
  // write image data
  switch (info_hdr.iCompression) {
  case BMPC_RGB:
    {
      uint8_t payload [file_stride];
      
      for (int row = image.h-1; row >=0; --row)
	{
	  memcpy (payload, image.getRawData() + stride*row, stride);
	  rearrangePixels (payload, image.w, info_hdr.iBitCount);
	  
	  if (!stream->write ((char*)payload, file_stride) ) {
	    std::cerr << "scanline " << row << " write error" << std::endl;
	  }
	}
    }
    break;
    
  default:
    std::cerr << "unsupported compression method writing bmp" << std::endl;
    return false;
  } /* switch */
  
  return true;
}

BMPCodec bmp_loader;
