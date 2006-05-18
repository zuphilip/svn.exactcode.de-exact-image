
#include <stdlib.h>

#include <gif_lib.h>

#include "gif.hh"
#include "Colorspace.hh"

#include <iostream>

/* The way Interlaced image should. */
static int InterlacedOffset[] = { 0, 4, 2, 1 };

/* be read - offsets and jumps... */
static int InterlacedJumps[] = { 8, 8, 4, 2 };

bool GIFLoader::readImage (FILE* file, Image& image)
{
  GifFileType* GifFile;
  GifByteType* Ptr;
  GifRecordType RecordType;
  GifByteType* Extension;
  ColorMapObject *ColorMap = NULL;
  int ExtCode;
  
  if ((GifFile = DGifOpenFileHandle(fileno(file))) == NULL)
    {
      PrintGifError();
      return false;
    }
  
  image.spp = 1;
  image.bps = 8;
  image.xres = 0;
  image.yres = 0;
  image.New (GifFile->SWidth,GifFile->SHeight);
  
  /* Scan the content of the GIF file and load the image(s) in: */
  do {
    if (DGifGetRecordType(GifFile, &RecordType) == GIF_ERROR) {
      PrintGifError();
      return false;
    }
    
    int Row, Col, Width, Height;
    
    switch (RecordType) {
    case IMAGE_DESC_RECORD_TYPE:
      if (DGifGetImageDesc(GifFile) == GIF_ERROR) {
	PrintGifError();
	return false;
      }
      
      Row = GifFile->Image.Top; /* Image Position relative to Screen. */
      Col = GifFile->Image.Left;
      Width = GifFile->Image.Width;
      Height = GifFile->Image.Height;
      
      if (GifFile->Image.Left + GifFile->Image.Width > GifFile->SWidth ||
	  GifFile->Image.Top + GifFile->Image.Height > GifFile->SHeight) {
	std::cerr << "Image not in screen dimension, aborted." << std::endl;
	return false;
      }
      if (GifFile->Image.Interlace) {
	/* Need to perform 4 passes on the images: */
	for (int i = 0; i < 4; ++i)
	  for (int j = Row + InterlacedOffset[i]; j < Row + Height;
	       j += InterlacedJumps[i]) {
	    if (DGifGetLine(GifFile, &image.data[j*image.Stride()+Col],
			    Width) == GIF_ERROR) {
	      PrintGifError();
	      return false;
	    }
	  }
      }
      else {
	for (int i = 0; i < Height; ++i) {
	  if (DGifGetLine(GifFile, &image.data[Row++ * image.Stride()+Col],
			  Width) == GIF_ERROR) {
	    PrintGifError();
	    return false;
	  }
	}
      }
      break;
    case EXTENSION_RECORD_TYPE:
      /* Skip any extension blocks in file: */
      if (DGifGetExtension(GifFile, &ExtCode, &Extension) == GIF_ERROR) {
	PrintGifError();
	return false;
      }
      while (Extension != NULL) {
	if (DGifGetExtensionNext(GifFile, &Extension) == GIF_ERROR) {
	  PrintGifError();
	  return false;
	}
      }
      break;
    case TERMINATE_RECORD_TYPE:
      break;
    default:		    /* Should be traps by DGifGetRecordType. */
      break;
    }
  }
  while (RecordType != TERMINATE_RECORD_TYPE);
  
  ColorMap = (GifFile->Image.ColorMap ? GifFile->Image.ColorMap :
	      GifFile->SColorMap);
  
  uint16_t rmap [ColorMap->ColorCount];
  uint16_t gmap [ColorMap->ColorCount];
  uint16_t bmap [ColorMap->ColorCount];
  for (int i = 0; i < ColorMap->ColorCount; ++i) {
    rmap[i] = ColorMap->Colors[i].Red << 8;
    gmap[i] = ColorMap->Colors[i].Green << 8;
    bmap[i] = ColorMap->Colors[i].Blue << 8;
  }
  
  // convert colormap to our 16bit "TIFF"format
  colorspace_de_palette (image, ColorMap->ColorCount, rmap, gmap, bmap);
  
  EGifCloseFile(GifFile);

  return true;
}

bool GIFLoader::writeImage (FILE* file, Image& image, int quality, const std::string& compress)
{
  GifFileType* GifFile;
  GifByteType* Ptr;
  
  if ((GifFile = EGifOpenFileHandle(fileno(file))) == NULL)
    {
      std::cerr << "Error preparing GIF file for writing." << std::endl;
      return false;
    }
#if 0 // create palette
  if (EGifPutScreenDesc(GifFile, image.w, image.h,
			ExpColorMapSize, 0, OutputColorMap) == GIF_ERROR ||
      EGifPutImageDesc(GifFile, 0, 0, image.w, image.h,
		       FALSE, NULL) == GIF_ERROR)
    {
      std::cerr << "Error writing GIF header." << std::endl;
      return false;
    }
  
  for (int i = 0; i < image.h; ++i) {
    if (EGifPutLine(GifFile, Ptr, image.w) == GIF_ERROR)
      {
	std::cerr << "Error writing GIF header." << std::endl;
	return false;
      }
    
    Ptr += image.w;
  }
#endif
  EGifCloseFile(GifFile);
  return false;
}

GIFLoader gif_loader;
