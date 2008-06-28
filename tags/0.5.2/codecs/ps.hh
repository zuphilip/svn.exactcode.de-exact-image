/*
 * Copyright (c) 2008 Valentin Ziegler <valentin@exactcode.de>
 * Copyright (c) 2008 Susanne Klaus <susanne@exactcode.de>
 *
*/

#include "Codecs.hh"

class PSCodec : public ImageCodec {
public:
  
  PSCodec () {
    registerCodec ("ps", this);
  };
  
  virtual std::string getID () { return "PS"; };
  
  virtual bool readImage (std::istream* stream, Image& image);
  virtual bool writeImage (std::ostream* stream, Image& image,
			   int quality, const std::string& compress);

  static void encodeImage (std::ostream* stream, Image& image, double scale,
			   int quality, const std::string& compress);
};
