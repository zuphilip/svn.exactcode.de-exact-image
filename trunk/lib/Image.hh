
#ifndef IMAGE_HH
#define IMAGE_HH

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
  
  void New (int _w, int _h) {
    w = _w;
    h = _h;
    data = (unsigned char*) malloc (Stride() * h);
  }
  
  int Stride () {
    return (w * spp * bps + 7) / 8;
  }
  
  typedef unsigned char* iterator;
  iterator begin () {
    return data;
  }
  
  iterator end () {
    return data + Stride() * h + 1;
  }
  
  
  int w, h, bps, spp, xres, yres;
  unsigned char* data;
};

typedef struct { unsigned char r, g, b; } rgb;

#endif
