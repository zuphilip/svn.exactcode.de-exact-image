/*
 * C++ BMP library.
 * Copyright (C) 2006 - 2016 René Rebe, ExactCODE GmbH Germany
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2. A copy of the GNU General
 * Public License can be found in the file LICENSE.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANT-
 * ABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details.
 *
 * Alternatively, commercial licensing options are available from the
 * copyright holder ExactCODE GmbH Germany.
 *
 * Lossy based on (in the past more so, but more and more parts got
 * rewritten):
 * Project:  libtiff tools
 * Purpose:  Convert Windows BMP files in TIFF.
 * Author:   Andrey Kiselev, dron@remotesensing.org
 */

#include <iostream>

#include "bmp.hh"

#include "Colorspace.hh"

#include <string.h>
#include <ctype.h>
#include <vector>

#include <Endianess.hh>
#include <stdint.h>
#include <Bits.hh>

using Exact::EndianessConverter;
using Exact::LittleEndianTraits;

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
    BMPC_RGB = 0,          /* Uncompressed */
    BMPC_RLE8 = 1,         /* RLE for 8 bpp images */
    BMPC_RLE4 = 2,         /* RLE for 4 bpp images */
    BMPC_BITFIELDS = 3,    /* Bitmap is not compressed and the colour table
			     * consists of three DWORD color masks that specify
			     * the red, green, and blue components of each
			     * pixel. This is valid when used with
			     * 16- and 32-bpp bitmaps. */
    BMPC_JPEG = 4,         /* Indicates that the image is a JPEG image. */
    BMPC_PNG = 5,           /* Indicates that the image is a PNG image. */
    BMPC_ALPHABITFIELDS = 6,
    BMPC_CMYK = 11,
    BMPC_CMYKRLE = 12,
    BMPC_CMYRTLE = 13,
    
  };

enum BMPLCSType                 /* Type of logical color space. */
  {
    BMPLT_CALIBRATED_RGB = 0,	/* This value indicates that endpoints and
				 * gamma values are given in the appropriate
				 * fields. */
    BMPLT_DEVICE_RGB = 1,
    BMPLT_DEVICE_CMYK = 2,
  };

#ifdef _MSC_VER
#pragma pack(push, 1)
#endif

struct BMPCIEXYZ
{
  int32_t   iCIEX;
  int32_t   iCIEY;
  int32_t   iCIEZ;
};

struct BMPCIEXYZTriple          /* This structure contains the x, y, and z */
{				/* coordinates of the three colors that */
				/* correspond */
  BMPCIEXYZ   iCIERed;        /* to the red, green, and blue endpoints for */
  BMPCIEXYZ   iCIEGreen;      /* a specified logical color space. */
  BMPCIEXYZ   iCIEBlue;
};


struct BMPFileHeader
{
  char	bType[2];       /* Signature "BM" */
  EndianessConverter<uint32_t,LittleEndianTraits> iSize; /* Size in bytes of the bitmap file. Should
				 * always be ignored while reading because
				 * of error in Windows 3.0 SDK's description
				 * of this field */
  uint16_t	iReserved1;     /* Reserved, set as 0 */
  uint16_t	iReserved2;     /* Reserved, set as 0 */
  EndianessConverter<uint32_t,LittleEndianTraits> iOffBits; /* Offset of the image from file start in bytes */
}
#ifdef __GNUC__
__attribute__((packed))
#endif
;


struct BMPInfoHeader
{
  // Fields used for bitmaps, compatible with Windows NT 3.51 and earlier.
  EndianessConverter<uint32_t,LittleEndianTraits> iSize; /* Size of BMPInfoHeader structure in bytes.
				 * Should be used to determine start of the
				 * colour table */
  EndianessConverter<int32_t,LittleEndianTraits> iWidth; /* Image width */
  EndianessConverter<int32_t,LittleEndianTraits> iHeight;        /* Image height. If positive, image has bottom
			 * left origin, if negative --- top left. */
  EndianessConverter<int16_t,LittleEndianTraits> iPlanes; /* Number of image planes (must be set to 1) */
  EndianessConverter<int16_t,LittleEndianTraits> iBitCount; /* Number of bits per pixel (1, 4, 8, 16, 24
				 * or 32). If 0 then the number of bits per
			 * pixel is specified or is implied by the
			 * JPEG or PNG format. */
  EndianessConverter<uint32_t,LittleEndianTraits> iCompression; /* Compression method */
  EndianessConverter<uint32_t,LittleEndianTraits> iSizeImage; /* Size of uncomressed image in bytes. May
				 * be 0 for BMPC_RGB bitmaps. If iCompression
				 * is BI_JPEG or BI_PNG, iSizeImage indicates
				 * the size of the JPEG or PNG image buffer. */
  EndianessConverter<int32_t,LittleEndianTraits> iXPelsPerMeter; /* X resolution, pixels per meter (0 if not used) */
  EndianessConverter<int32_t,LittleEndianTraits> iYPelsPerMeter; /* Y resolution, pixels per meter (0 if not used) */
  EndianessConverter<uint32_t,LittleEndianTraits> iClrUsed; /* Size of colour table. If 0, iBitCount should
				 * be used to calculate this value
				 * (1<<iBitCount). This value should be
				 * unsigned for proper shifting. */
  EndianessConverter<int32_t,LittleEndianTraits> iClrImportant;  /* Number of important colours. If 0, all
			 * colours are required */
  // 40 bytes
  
  // Windows 98/Me, Windows 2000/XP introduces additional fields:
  EndianessConverter<uint32_t,LittleEndianTraits> iRedMask;       /* Colour mask that specifies the red component
			 * of each pixel, valid only if iCompression
			 * is set to BI_BITFIELDS. */
  EndianessConverter<uint32_t,LittleEndianTraits> iGreenMask;     /* The same for green component */
  EndianessConverter<uint32_t,LittleEndianTraits> iBlueMask;      /* The same for blue component */
  EndianessConverter<uint32_t,LittleEndianTraits> iAlphaMask;     /* Colour mask that specifies the alpha
			 * component of each pixel. */
  // 56 bytes
  
  EndianessConverter<uint32_t,LittleEndianTraits> iCSType;        /* Colour space of the DIB. */
  BMPCIEXYZTriple sEndpoints; /* This member is ignored unless the iCSType
			       * member specifies BMPLT_CALIBRATED_RGB. */
  EndianessConverter<int32_t,LittleEndianTraits> iGammaRed;      /* Toned response curve for red. This member
			 * is ignored unless color values are
			 * calibrated RGB values and iCSType is set to
			 * BMPLT_CALIBRATED_RGB. Specified
			 * in 16^16 format. */
  EndianessConverter<int32_t,LittleEndianTraits> iGammaGreen;    /* Toned response curve for green. */
  EndianessConverter<int32_t,LittleEndianTraits> iGammaBlue;     /* Toned response curve for blue. */
  // 108 bytes
}
#ifdef __GNUC__
__attribute__((packed))
#endif
;

// Info header size in bytes:
static const unsigned BIH_OS21SIZE = 12; // OS/2, Windows 2
static const unsigned BIH_OS22SIZE = 64;
static const unsigned BIH_V1SIZE = 40; // Win NT, 3.1
static const unsigned BIH_V2SIZE = 52;
static const unsigned BIH_V3SIZE = 56;
static const unsigned BIH_V4SIZE = 108; // Win NT 4.0, 95
static const unsigned BIH_V5SIZE = 124; // Win NT 5.0, 98

// We will use plain byte array instead of this structure, for reference:
struct BMPColorEntry
{
  char       bBlue;
  char       bGreen;
  char       bRed;
  char       bReserved; // Must be 0
};

#ifdef _MSC_VER
#pragma pack(pop)
#endif

// Image data in BMP file stored in BGR (or ABGR) format:
static void rearrangePixels(uint8_t* buf, uint32_t width, uint32_t bit_count)
{
  switch (bit_count) {
  case 16:    // FIXME: need a sample file
    break;
    
  case 24:
    for (uint32_t i = 0; i < width; i++, buf += 3)
      std::swap(buf[0], buf[2]);
    break;

  case 48:
    {
      uint16_t* buf16 = (uint16_t*)buf;
      for (uint32_t i = 0; i < width; i++, buf16 += 3)
	std::swap(buf16[0], buf16[2]);
    }
    break;

  case 32:
    {
      for (uint32_t i = 0; i < width; i++, buf += 4)
	std::swap(buf[0], buf[2]);
    }
    break;
    
  default:
    // sub-byte color table rows remain unchanged
    break;
  }
}

int BMPCodec::readImage (std::istream* stream, Image& image, const std::string& decompres)
{
  BMPFileHeader file_hdr;
  
  stream->read ((char*)&file_hdr.bType, 2);
  if (file_hdr.bType[0] != 'B' || file_hdr.bType[1] != 'M') {
    stream->seekg (0);
    return false;
  }
  
  // Read the BMPFileHeader. We need iOffBits value only.
  stream->seekg (10);
  stream->read ((char*)&file_hdr.iOffBits, 4);
  
  // fix the iSize, in early BMP file this is pure garbage
  stream->seekg (0, std::ios::end);
  file_hdr.iSize = stream->tellg(); // TODO: minus the header?

  int i = readImageWithoutFileHeader(stream, image, decompres, &file_hdr);
  return i;
}

int BMPCodec::readImageWithoutFileHeader (std::istream* stream, Image& image, const std::string& decompres, BMPFileHeader* _file_hdr)
{
  BMPFileHeader* file_hdr = _file_hdr;
  BMPFileHeader file_header = {}; // only used if no file_hdr is supplied
  BMPInfoHeader info_hdr = {};
  int offset = file_hdr ? sizeof(*file_hdr) : 0;
  
  uint32_t clr_tbl_size = 0, n_clr_elems = 3;
  uint8_t* clr_tbl = 0;
  
  // Read the BMPInfoHeader
  stream->seekg (offset);
  stream->read ((char*)&info_hdr.iSize, 4);
  
  if (!_file_hdr) {
    offset = 0;
    file_hdr = &file_header;
    stream->seekg (0, std::ios::end);
    file_hdr->iSize = stream->tellg();
    file_hdr->iOffBits = info_hdr.iSize; // assumed to follow after info header
    
    stream->seekg (offset + 4);
  }
  
  unsigned iSize = info_hdr.iSize;
  switch (iSize) {
  case 16:
    iSize = BIH_OS22SIZE; break;
  case BIH_OS21SIZE:
  case BIH_OS22SIZE:
  case BIH_V1SIZE:
  case BIH_V2SIZE:
  case BIH_V3SIZE:
  case BIH_V4SIZE:
  case BIH_V5SIZE:
    break;
  default:
    std::cerr << "Unknown header size: " << info_hdr.iSize << std::endl;
    return false;
  }
  
  if (iSize == BIH_OS21SIZE) {
    int16_t  iShort;
    stream->read ((char*)&iShort, 2);
    info_hdr.iWidth = iShort;
    stream->read ((char*)&iShort, 2);
    info_hdr.iHeight = iShort;
    stream->read ((char*)&iShort, 2);
    info_hdr.iPlanes = iShort;
    stream->read ((char*)&iShort, 2);
    info_hdr.iBitCount = iShort;
    info_hdr.iCompression = BMPC_RGB;
    n_clr_elems = 3;
  }
  else{
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
    n_clr_elems = 4;
    
    if (iSize >= BIH_V2SIZE ||
	info_hdr.iCompression == BMPC_BITFIELDS ||
	info_hdr.iCompression == BMPC_ALPHABITFIELDS) {
      stream->read((char*)&info_hdr.iRedMask, 4);
      stream->read((char*)&info_hdr.iGreenMask, 4);
      stream->read((char*)&info_hdr.iBlueMask, 4);
      if (iSize >= BIH_V3SIZE || info_hdr.iCompression == BMPC_ALPHABITFIELDS)
	stream->read((char*)&info_hdr.iAlphaMask, 4);
      
      /*std::cerr << std::hex << "red mask: " << info_hdr.iRedMask
		  << ", green mask: " << info_hdr.iGreenMask
		  << ", blue mask: " << info_hdr.iBlueMask
		  << ", alpha mask: " << info_hdr.iAlphaMask << std::dec << std::endl;
      */
    }
  }
  
  //std::cerr << "size: " << iSize << ", bits: " << info_hdr.iBitCount << ", compr: " << info_hdr.iCompression << std::endl;
  
  if (iSize == BIH_OS22SIZE) {
    //FIXME: different info in different documents regarding this!
    n_clr_elems = 3;
  }

  image.setResolution((2.54 * info_hdr.iXPelsPerMeter) / 100 + .5,
		      (2.54 * info_hdr.iYPelsPerMeter) / 100 + .5);
  
  switch (info_hdr.iBitCount) {
  case 1:
  case 2: // valid in Win CE?
  case 4:
  case 8:
  case 16:
  case 24:
  case 32:
  case 48:
    break;
  default:
    std::cerr << "BMPCodec:: Cannot read " << info_hdr.iBitCount
	      << " bit files." << std::endl;
    return false;
  }

  image.w = info_hdr.iWidth;
  image.h = std::abs(info_hdr.iHeight); // negative when upside-down
  
  switch (info_hdr.iBitCount)
    {
    case 1:
    case 2:
    case 4:
    case 8:
      image.spp = 1;
      image.bps = info_hdr.iBitCount;

      // Allocate memory for colour table and read it.
      if (info_hdr.iClrUsed)
	clr_tbl_size =
	  ((uint32_t)(1 << image.bps) < info_hdr.iClrUsed) ? 1 << image.bps : info_hdr.iClrUsed;
      else
	clr_tbl_size = 1 << image.bps;

      //std::cerr << "n_clr_elems: " << n_clr_elems << ", clr_tbl_size: " << clr_tbl_size << std::endl;

      // if we had no file_hdr, update offset to compensate for color table
      if (!_file_hdr) {
        file_hdr->iOffBits = file_hdr->iOffBits + n_clr_elems * clr_tbl_size;
      }

      clr_tbl = (uint8_t*)malloc(n_clr_elems * clr_tbl_size);
      if (!clr_tbl) {
	std::cerr << "BMPCodec:: Can't allocate space for color table" << std::endl;
	return false;
      }
      
      stream->seekg (offset + info_hdr.iSize);
      stream->read ((char*)clr_tbl, n_clr_elems * clr_tbl_size);
      
      /*for(clr = 0; clr < clr_tbl_size; ++clr) {
	printf ("%d: r: %d g: %d b: %d\n", clr,
		clr_tbl[clr*n_clr_elems+2], clr_tbl[clr*n_clr_elems+1], clr_tbl[clr*n_clr_elems]);
      }*/
      break;
    
    case 16:
    case 24:
    case 32:
      image.spp = 3;
      image.bps = 8;
      
      if (info_hdr.iCompression == BMPC_ALPHABITFIELDS)
	image.spp = 4;
      else if (iSize >= BIH_V2SIZE && info_hdr.iCompression == BMPC_BITFIELDS && info_hdr.iAlphaMask != 0)
	image.spp = 4; // TODO: does gray + alpha exist?
      else if (info_hdr.iCompression == BMPC_RGB && info_hdr.iBitCount == 32)
	image.spp = 4;
      
      break;
    
    case 48:
      image.spp = 3;
      image.bps = 16;
      break;

    default:
      break;
    }
  
  // detect old style bitmask images
  if (info_hdr.iCompression == BMPC_RGB && info_hdr.iBitCount == 16)
    {
      //std::cerr << "implicit non-RGB image\n";
      info_hdr.iCompression = BMPC_BITFIELDS;
      info_hdr.iBlueMask = 0x1f;
      info_hdr.iGreenMask = 0x1f << 5;
      info_hdr.iRedMask = 0x1f << 10;
      info_hdr.iAlphaMask = 0;
    }
  
  unsigned stride = image.stride();
  
  // Read uncompressed image data.
  switch (info_hdr.iCompression) {
  case BMPC_BITFIELDS:
  case BMPC_ALPHABITFIELDS:
    image.bps = 8; // we unpack bitfields to plain RGB
    stride = image.stride();
    
  case BMPC_RGB:
    {
      uint32_t file_stride = ((image.w * info_hdr.iBitCount + 7) / 8 + 3) / 4 * 4;
      
      /*std::cerr << "bitcount: " << info_hdr.iBitCount << ", stride: " << stride
                << ", file stride: " << file_stride << std::endl;
      if (info_hdr.iCompression == BMPC_BITFIELDS || info_hdr.iCompression == BMPC_ALPHABITFIELDS)
	std::cerr << std::hex << "red mask: " << info_hdr.iRedMask
		  << ", green mask: " << info_hdr.iGreenMask
		  << ", blue mask: " << info_hdr.iBlueMask
		  << ", alpha mask: " << info_hdr.iAlphaMask << std::dec << std::endl;
      */
      if (file_stride > stride)
	stride = file_stride; // use the BMP's native stride
      image.resize(image.w, image.h, stride);
      
      const int r_bits = Exact::popcountf(info_hdr.iRedMask);
      const int r_shift = Exact::ms_bit_set(info_hdr.iRedMask) + 1 - r_bits;
      const int g_bits = Exact::popcountf(info_hdr.iGreenMask);
      const int g_shift = Exact::ms_bit_set(info_hdr.iGreenMask) + 1 - g_bits;
      const int b_bits = Exact::popcountf(info_hdr.iBlueMask);
      const int b_shift = Exact::ms_bit_set(info_hdr.iBlueMask) + 1 - b_bits;
      const int a_bits = Exact::popcountf(info_hdr.iAlphaMask);
      const int a_shift = Exact::ms_bit_set(info_hdr.iAlphaMask) + 1 - a_bits;

      /*std::cerr << r_bits << " " << r_shift << " " << g_bits << " " << g_shift << " "
		<< b_bits << " " << b_shift << " " << a_bits << " " << a_shift << std::endl;
      */
      uint8_t* data = image.getRawData();
      for (uint32_t row = 0; row < (uint32_t)image.h; ++row)
      {
	std::istream::pos_type offset = file_hdr->iOffBits + row * file_stride;
	stream->seekg(offset);
	
	if (stream->tellg() != offset) {
	  std::cerr << "scanline " << row << " Seek error: " << stream->tellg() << " vs " << offset << std::endl;
	}
	
	uint8_t* row_ptr = data +
	  stride * (info_hdr.iHeight < 0 ? row : image.h - row - 1);
	
	stream->read((char*)row_ptr, file_stride);
	if (!stream->good()) {
	  std::cerr << "bmp read error: scanline " << row << "\n";
	} else {
	  // convert to RGB
	  if (info_hdr.iCompression == BMPC_BITFIELDS || info_hdr.iCompression == BMPC_ALPHABITFIELDS)
	  {
	    const int spp = image.spp;
	    int beg = 0, end = image.w; int8_t inc = 1;
	    if (image.spp * image.bps > info_hdr.iBitCount) {
	      // reverse, not to clobber in-line data
	      beg = image.w - 1;
	      end = -1;
	      inc = -1;
	    }
	    
	    //if (row == 0) std::cerr << beg << " " << end << " " << (int)inc << std::endl;
	    for (int i = beg; i != end; i += inc)
	      {
		uint8_t* bf_ptr = row_ptr + i * info_hdr.iBitCount / 8;
		int32_t val = *bf_ptr++; // 1st 8 bits
		for (int bits = 8; bits < info_hdr.iBitCount; bits += 8)
		  val |= (*bf_ptr++) << bits;
		    
		row_ptr[i*spp + 0] =
		  ((val & info_hdr.iRedMask) >> r_shift) * 0xff / ((1 << r_bits) - 1);
		row_ptr[i*spp + 1] =
		  ((val & info_hdr.iGreenMask) >> g_shift) * 0xff / ((1 << g_bits) - 1);
		row_ptr[i*spp + 2] =
		  ((val & info_hdr.iBlueMask) >> b_shift) * 0xff / ((1 << b_bits) - 1);
		if (spp > 3)
		  row_ptr[i*spp + 3] = 
		    ((val & info_hdr.iAlphaMask) >> a_shift) * 0xff / ((1 << a_bits) - 1);
	      }
	  } else {
	    rearrangePixels (row_ptr, image.w, info_hdr.iBitCount);
	  }
	}
      }
    }
    break;
    
    // Read compressed image data
  case BMPC_RLE4:
  case BMPC_RLE8:
    {
      const unsigned compr_size = file_hdr->iSize - file_hdr->iOffBits;
      const unsigned uncompr_size = image.w * image.h;
      
      image.bps = 8;
      image.resize(image.w, image.h);
      uint8_t* uncomprbuf = image.getRawData();
      memset(uncomprbuf, 0, uncompr_size); // for skipped pixels, ...
      
#ifdef _MSC_VER
      std::vector<uint8_t> comprbuf(compr_size);
#else
      uint8_t comprbuf[compr_size];
#endif
      stream->seekg(*file_hdr->iOffBits);
      stream->read((char*)&comprbuf[0], compr_size);
      
      int y = image.h - 1;
      uint8_t* rowptr = uncomprbuf + y * image.w;
      for (unsigned i = 0, x = 0; y >= 0 && i < compr_size;) {
	if (comprbuf[i]) {
	  uint8_t runlength = comprbuf[i++];
	  for (unsigned k = 0;
	       runlength > 0 && i < compr_size && x < (uint32_t)image.w;
	       ++k) {
	    if (info_hdr.iBitCount == 8)
	      rowptr[x++] = comprbuf[i];
	    else {
	      if (k & 1)
		rowptr[x++] = comprbuf[i] & 0x0F;
	      else
		rowptr[x++] = (comprbuf[i] & 0xF0) >> 4;
	    }
	    runlength--;
	  }
	  ++i;
	} else {
	  ++i;
	  uint8_t v = comprbuf[i];
	  if (v == 0) { // Next scanline
	    ++i;
	    x = 0; --y;
	    rowptr = uncomprbuf + y * image.w;
	  }
	  else if (v == 1) // End of image
	    break;
	  else if (v == 2) { // Move to...
	    ++i;
	    if (i < compr_size - 1) {
	      x += comprbuf[i];
	      y -= comprbuf[i+1];
	      rowptr = uncomprbuf + y * image.w;
	      i += 2;
	    }
	    else
	      break;
	  } else { // uncompressed mode
	    uint8_t runlength = comprbuf[i++];
	    for (unsigned k = 0; k < runlength && x < image.w && i < compr_size; ++k)
	      {
		if (info_hdr.iBitCount == 8)
		  rowptr[x++] = comprbuf[i++];
		else {
		  if (k & 1)
		    rowptr[x++] = v & 0x0F;
		  else {
		    v = comprbuf[i++]; // store for next nibble
		    rowptr[x++] = (v & 0xF0) >> 4;
		  }
		}
	      }
	    
	    // word boundary alignment
 	    if (i & 1)
	      ++i;
	  }
	}
      }
    }
    break;
    
  default:
    std::cerr << "BMPCodec: unsuppored compression: " << info_hdr.iCompression << std::endl;
  
  }
  
  // convert to RGB color-space - we do not handle palette images internally
  
  // no color table anyway or RGB* ?
  if (clr_tbl && image.spp < 3)
    {
      uint16_t* rmap = new uint16_t [clr_tbl_size];
      uint16_t* gmap = new uint16_t [clr_tbl_size];
      uint16_t* bmap = new uint16_t [clr_tbl_size];
      
      for (unsigned int i = 0; i < clr_tbl_size; ++i) {
	// BMP maps have BGR order ...
	rmap[i] = 0x101 * clr_tbl[i * n_clr_elems + 2];
	gmap[i] = 0x101 * clr_tbl[i * n_clr_elems + 1];
	bmap[i] = 0x101 * clr_tbl[i * n_clr_elems + 0];
      }
      
      colorspace_de_palette (image, clr_tbl_size, rmap, gmap, bmap);
      
      delete[] (rmap);
      delete[] (gmap);
      delete[] (bmap);
      
      free(clr_tbl);
      clr_tbl = NULL;
    }
  else if (image.spp == 4 && (iSize < BIH_V2SIZE && info_hdr.iCompression == BMPC_RGB)) {
    colorspace_rgba8_to_rgb8(image);
  }
  
  return true;
}

bool BMPCodec::writeImage (std::ostream* stream, Image& image, int quality,
			   const std::string& compress)
{
  const int hdr_size = image.spp == 4 ? BIH_V3SIZE : BIH_V1SIZE;
  const unsigned stride = image.stride();
  const int n_clr_elems = 4; // we write "modern" formats, not the vintage OS/2 flavour
  
  if (image.bps > 16 || image.spp > 4) {
    std::cerr << "BMPCodec: " << image.bps << " bits and " << image.spp << " samples not supported." << std::endl;
    return false;
  }
  
  BMPFileHeader file_hdr = {};
  BMPInfoHeader info_hdr = {};

  // BMPFileHeader
  file_hdr.bType[0] = 'B';
  file_hdr.bType[1] = 'M';
  
  // BMPInfoHeader
  info_hdr.iSize = hdr_size;
  info_hdr.iWidth = image.w;
  info_hdr.iHeight = image.h;
  info_hdr.iPlanes = 1;
  info_hdr.iBitCount = image.spp * image.bps;
  info_hdr.iCompression = BMPC_RGB;
  const int file_stride = ((image.w * info_hdr.iBitCount + 7) / 8 + 3) / 4 * 4; // BMP 4byte aligned
  info_hdr.iSizeImage  = file_stride * image.h; // TODO: compressed size
  info_hdr.iXPelsPerMeter = (int32_t) (100. * image.resolutionX() / 2.54 + .5);
  info_hdr.iYPelsPerMeter = (int32_t) (100. * image.resolutionY() / 2.54 + .5);
  info_hdr.iClrUsed = image.spp == 1 ? 1 << image.bps : 0;
  info_hdr.iClrImportant = 0;
  info_hdr.iRedMask = 0;
  info_hdr.iGreenMask = 0;
  info_hdr.iBlueMask = 0;
  info_hdr.iAlphaMask = 0;
  
  
  file_hdr.iOffBits = sizeof(file_hdr) + hdr_size + info_hdr.iClrUsed * n_clr_elems;
  file_hdr.iSize =  file_hdr.iOffBits + file_stride * image.h;
  
  // write header meta info
  stream->write ((char*)&file_hdr, sizeof(file_hdr));
  stream->write ((char*)&info_hdr, hdr_size);
  
  // write color table
  if (info_hdr.iClrUsed) {
    int n = info_hdr.iClrUsed;
#ifdef _MSC_VER
    std::vector<uint8_t> _clrtbl(n_clr_elems * n);
    uint8_t* clrtbl = &_clrtbl[0];
#else
    uint8_t clrtbl [n_clr_elems*n];
#endif
    for (int i = 0; i < n; ++i) {
      clrtbl[n_clr_elems*i+0] = clrtbl[n_clr_elems*i+1] = clrtbl[n_clr_elems*i+2] = i * 0xff / (n - 1);
      
      for (int j = 3; j < n_clr_elems; ++j)
	clrtbl[n_clr_elems*i+j] = 0;
    }
    stream->write ((char*)clrtbl, n_clr_elems*n);
  }
  
  // write image data
  switch (info_hdr.iCompression) {
  case BMPC_RGB:
    {
#ifdef _MSC_VER
      std::vector<uint8_t> payload(file_stride);
#else
      uint8_t payload[file_stride];
#endif
      for (int i = stride; i < file_stride; ++i)
	payload[i] = 0; // zero initialize padding
      for (int row = image.h-1; row >= 0; --row)
	{
	  memcpy(&payload[0], image.getRawData() + stride * row, stride);
	  rearrangePixels(&payload[0], image.w, info_hdr.iBitCount);
	  
	  if (!stream->write((char*)&payload[0], file_stride)) {
	    std::cerr << "scanline " << row << " write error" << std::endl;
	    return false;
	  }
	}
    }
    break;
    
  default:
    std::cerr << "unsupported compression method writing bmp" << std::endl;
    return false;
  }
  
  return true;
}

BMPCodec bmp_loader;
