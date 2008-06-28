/*
 *
 * Copyright (c) 2008 Susanne Klaus <susanne@exactcode.de>
 *
*/

#include "Codecs.hh"

class PDFCodec : public ImageCodec {
public:
  
  PDFCodec () {
    registerCodec ("pdf", this);
  };
  
  virtual std::string getID () { return "PDF"; };
  
  virtual bool readImage (std::istream* stream, Image& image);
  virtual bool writeImage (std::ostream* stream, Image& image,
			   int quality, const std::string& compress);
};
