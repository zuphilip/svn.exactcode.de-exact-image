
#include <string>
  
class Image
{
public:

  Image ()
    : data(0) {
  }
  
  ~Image () {
    if (data)
      free (data);
  }
  
  bool Read (const std::string& file);
  bool Write (const std::string& file);
  
  int Stride () {
    return (w * spp * bps + 7) / 8;
  }
  
  int w, h, bps, spp, xres, yres;
  unsigned char* data;
};

typedef struct { unsigned char r, g, b; } rgb;
