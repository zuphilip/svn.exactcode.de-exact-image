
#ifndef IMAGE_HH
#define IMAGE_HH

#include <string>

class Image
{
public:
  
  typedef enum {
    BILEVEL,
    GRAY,
    GRAY16,
    RGB,
    RGB16,
    CYMK,
    YUV
  } type_t;

  typedef union {
    u_int8_t gray;
    u_int16_t gray16;
    struct {
      u_int8_t r;
      u_int8_t g;
      u_int8_t b;
    } rgb;
    struct {
      u_int16_t r;
      u_int16_t g;
      u_int16_t b;
    } rgb16;
    struct {
      u_int8_t c;
      u_int8_t m;
      u_int8_t y;
      u_int8_t k;
    } cmyk;
    struct {
      u_int8_t y;
      u_int8_t u;
      u_int8_t v;
    } yuv;
  } value_t;
    
  typedef union {
    u_int32_t gray;
    struct {
      u_int32_t r;
      u_int32_t g;
      u_int32_t b;
    } rgb;
    struct {
      u_int32_t c;
      u_int32_t m;
      u_int32_t y;
      u_int32_t k;
    } cmyk;
    struct {
      u_int32_t y;
      u_int32_t u;
      u_int32_t v;
    } yuv;
  } ivalue_t;

  int w, h, bps, spp, xres, yres;
  unsigned char* data;
      
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
  
  int Stride () const {
    return (w * spp * bps + 7) / 8;
  }

  type_t Type () const {
    switch (spp*bps) {
    case 1:  return BILEVEL;
    case 8:  return GRAY;
    case 16: return GRAY16;
    case 24: return RGB;
    case 48: return RGB16;
    default:
      std::cerr << "Unknown type" << std::endl;
    }
  }
  
  class iterator
  {
  public:
    Image* image;
    
    type_t type;
    int stride;
    ivalue_t value;

    value_t* ptr;
    signed int bitpos; // for 1bps sub-position
    
    iterator (Image* _image, bool end)
      : image (_image), type (_image->Type()), stride (_image->Stride())
    {
      if (!end)
	ptr = (value_t*) image->data;
      else
	ptr = (value_t*) image->data + stride * image->h + image->w;
    }
    
    inline void clear () {
      switch (type) {
      case BILEVEL:
      case GRAY:
      case GRAY16:
	ptr->gray = 0;
	break;
      case RGB:
      case RGB16:
	ptr->rgb.r = ptr->rgb.g = ptr->rgb.b = 0;
	break;
      }
    }
    
    inline iterator at (int x, int y) {
      iterator tmp = *this;
      
      switch (type) {
      case BILEVEL:
	tmp.ptr = (value_t*) (image->data + stride * y + x / 8);
	bitpos = x % 8;
	tmp.value.gray = tmp.ptr->gray & (1 << bitpos) ? 255 : 0;
	break;
      case GRAY:
	tmp.ptr = (value_t*) (image->data + stride * y + x);
	tmp.value.gray = tmp.ptr->gray;
	break;
      case GRAY16:
	tmp.ptr = (value_t*) (image->data + stride * y + x * 2);
	tmp.value.gray = tmp.ptr->gray16;
	break;
      case RGB:
	tmp.ptr = (value_t*) (image->data + stride * y + x * 3);
	tmp.value.rgb.r = tmp.ptr->rgb.r;
	tmp.value.rgb.g = tmp.ptr->rgb.g;
	tmp.value.rgb.b = tmp.ptr->rgb.b;
	break;
      case RGB16:
	tmp.ptr = (value_t*) (image->data + stride * y + x * 6);
	tmp.value.rgb.r = tmp.ptr->rgb16.r;
	tmp.value.rgb.g = tmp.ptr->rgb16.g;
	tmp.value.rgb.b = tmp.ptr->rgb16.b;
	break;
      }
      return tmp;
    }
    
    inline iterator& operator+ (const iterator& other) {
      switch (type) {
      case BILEVEL:
      case GRAY:
      case GRAY16:
	value.gray += other.value.gray;
	break;
      case RGB:
      case RGB16:
	value.rgb.r += other.value.rgb.r;
	value.rgb.g += other.value.rgb.g;
	value.rgb.b += other.value.rgb.b;
	break;
      }
      return *this;
    }
    
    inline iterator& operator- (const iterator& other)  {
      switch (type) {
      case BILEVEL:
      case GRAY:
      case GRAY16:
	value.gray -= other.value.gray;
	break;
      case RGB:
      case RGB16:
	value.rgb.r -= other.value.rgb.r;
	value.rgb.g -= other.value.rgb.g;
	value.rgb.b -= other.value.rgb.b;
	break;
      }
      return *this;
    }
    
    inline iterator& operator* (const int v) {
      switch (type) {
      case BILEVEL:
      case GRAY:
      case GRAY16:
	value.gray *= v;
	break;
      case RGB:
      case RGB16:
	value.rgb.r *= v;
	value.rgb.g *= v;
	value.rgb.b *= v;
	break;
      }
      return *this;
    }
    
    inline iterator& operator/ (const int v) {
      switch (type) {
      case BILEVEL:
      case GRAY:
      case GRAY16:
	value.gray /= v;
	break;
      case RGB:
      case RGB16:
	value.rgb.r /= v;
	value.rgb.g /= v;
	value.rgb.b /= v;
	break;
      }
      return *this;
    }
    
    //prefix
    inline iterator& operator++ () {
      switch (type) {
      case BILEVEL:
	++bitpos;
	if (bitpos == 8) {
	  bitpos = 0;
	  ptr = (value_t*) ((u_int8_t*) ptr + 1);
	}
	break;
      case GRAY:
	ptr = (value_t*) ((u_int8_t*) ptr + 1); break;
      case GRAY16:
	ptr = (value_t*) ((u_int8_t*) ptr + 2); break;
      case RGB:
	ptr = (value_t*) ((u_int8_t*) ptr + 3); break;
      case RGB16:
	ptr = (value_t*) ((u_int8_t*) ptr + 6); break;
      }
      return *this;
    }
    
    inline iterator& operator-- () {
      switch (type) {
      case BILEVEL:
	--bitpos;
	if (bitpos < 0) {
	  bitpos = 7;
	  ptr = (value_t*) ((u_int8_t*) ptr - 1);
	}
	break;
      case GRAY:
	ptr = (value_t*) ((u_int8_t*) ptr - 1); break;
      case GRAY16:
	ptr = (value_t*) ((u_int8_t*) ptr - 2); break;
      case RGB:
	ptr = (value_t*) ((u_int8_t*) ptr - 3); break;
      case RGB16:
	ptr = (value_t*) ((u_int8_t*) ptr - 6); break;
      }
      return *this;
    }

    //postfix
#if 0
    inline iterator operator++ (int) {
      iterator tmp = *this;
      switch (type) {
      default:
	ptr = (value_t*) ((u_int8_t*) ptr + 1);
      }
      return tmp;
    }
    
    inline iterator operator-- (int) {
      iterator tmp = *this;
      switch (type) {
      default:
	ptr = (value_t*) ((u_int8_t*) ptr - 1);
      }
      return tmp;
    }
#endif
    
    inline void set (const iterator& other) {
      switch (type) {
      case BILEVEL:
	{
	  int i = other.value.gray > 127 ? 1 : 0;
	  ptr->gray |= i << bitpos;
	}
	break;
      case GRAY:
	ptr->gray = other.value.gray;
	break;
      case GRAY16:
	ptr->gray16 = other.value.gray;
	break;
      case RGB:
	ptr->rgb.r = other.value.rgb.r;
	ptr->rgb.g = other.value.rgb.g;
	ptr->rgb.b = other.value.rgb.b;
	break;
      case RGB16:
	ptr->rgb16.r = other.value.rgb.r;
	ptr->rgb16.g = other.value.rgb.g;
	ptr->rgb16.b = other.value.rgb.b;
	break;
      }
    }
  };
  
  iterator begin () {
    return iterator(this, false);
  }
  
  iterator end () {
    return iterator(this, true);
  }
};

typedef struct { unsigned char r, g, b; } rgb;

#endif
