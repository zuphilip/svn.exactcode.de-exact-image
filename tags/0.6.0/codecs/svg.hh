/*
 * Copyright (c) 2008 Rene Rebe <rene@exactcode.de>
 *
 */

#include "Codecs.hh"

class SVGCodec : public ImageCodec {
public:
  
  SVGCodec () {
    registerCodec ("svg", this);
  };
  
  virtual std::string getID () { return "SVG"; };
  
  virtual bool readImage (std::istream* stream, Image& image);
  virtual bool writeImage (std::ostream* stream, Image& image,
			   int quality, const std::string& compress);
};
