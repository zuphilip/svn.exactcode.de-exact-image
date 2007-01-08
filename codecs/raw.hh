#include "Codecs.hh"

class RAWCodec : public ImageCodec {
public:
  
  RAWCodec () { registerCodec ("raw", this, true /* explicit only */); };
  virtual ~RAWCodec () { unregisterCodec (this); };
  
  virtual std::string getID () { return "RAW data"; };
  
  virtual bool readImage (std::istream* stream, Image& image);
  virtual bool writeImage (std::ostream* stream, Image& image, int quality,
			   const std::string& compress);
};
