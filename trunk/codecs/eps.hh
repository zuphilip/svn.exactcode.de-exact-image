/*
 * Copyright (c) 2008 Valentin Ziegler <valentin@exactcode.de>
 * Copyright (c) 2008 Susanne Klaus <susanne@exactcode.de>
 *
 */

#include "Codecs.hh"

class EPSCodec : public ImageCodec {
public:
  
  EPSCodec () {
    registerCodec ("eps", this);
  };
  
  virtual std::string getID () { return "EPS"; };
  
  virtual bool readImage (std::istream* stream, Image& image);
  virtual bool writeImage (std::ostream* stream, Image& image,
			   int quality, const std::string& compress);
};
