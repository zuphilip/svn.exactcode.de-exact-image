
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

  int w, h, bps, spp, xres, yres;
  unsigned char* data;
};

typedef struct { unsigned char r, g, b; } rgb;
