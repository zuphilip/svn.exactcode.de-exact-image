
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
    CMYK,
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
    int stride, width, _x;
    ivalue_t value;

    value_t* ptr;
    signed int bitpos; // for 1bps sub-position
    
    // for seperate use, e.g. to accumulate
    iterator () {};
    
    iterator (Image* _image, bool end)
      : image (_image), type (_image->Type()),
	stride (_image->Stride()), width (image->w)
    {
      if (!end) {
	ptr = (value_t*) image->data;
	_x = 0;
	bitpos = 7;
      }
      else {
	ptr = (value_t*) image->data + stride * image->h + image->w;
	_x = width;
	// TODO: bitpos= ...
      }
    }
    
    inline void clear () {
      switch (type) {
      case BILEVEL:
      case GRAY:
      case GRAY16:
	value.gray = 0;
	break;
      case RGB:
      case RGB16:
	value.rgb.r = value.rgb.g = value.rgb.b = 0;
	break;
      case CMYK:
	value.cmyk.c = value.cmyk.m = value.cmyk.y = value.cmyk.k = 0;
	break;
      case YUV:
	value.yuv.y = value.yuv.u = value.yuv.v = 0;
	break;
      }
    }
    
    inline iterator at (int x, int y) {
      iterator tmp = *this;
      
      switch (type) {
      case BILEVEL:
	tmp.ptr = (value_t*) (image->data + stride * y + x / 8);
	tmp.bitpos = 7 - x % 8;
	tmp.value.gray = (tmp.ptr->gray & (1 << tmp.bitpos))
	  << (7 - tmp.bitpos); 
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
      case CMYK:
	tmp.value.cmyk.c = tmp.ptr->cmyk.c;
	tmp.value.cmyk.m = tmp.ptr->cmyk.m;
	tmp.value.cmyk.y = tmp.ptr->cmyk.y;
	tmp.value.cmyk.k = tmp.ptr->cmyk.k;
	break;
      case YUV:
	tmp.value.yuv.y = tmp.ptr->yuv.y;
	tmp.value.yuv.u = tmp.ptr->yuv.u;
	tmp.value.yuv.v = tmp.ptr->yuv.v;
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
      case CMYK:
	value.cmyk.c += other.value.cmyk.c;
	value.cmyk.m += other.value.cmyk.m;
	value.cmyk.y += other.value.cmyk.y;
	value.cmyk.k += other.value.cmyk.k;
	break;
      case YUV:
	value.yuv.y += other.value.yuv.y;
	value.yuv.u += other.value.yuv.u;
	value.yuv.v += other.value.yuv.v;
	break;     
      }
      return *this;
    }
    
    inline iterator& operator+= (const iterator& other) {
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
      case CMYK:
	value.cmyk.c += other.value.cmyk.c;
	value.cmyk.m += other.value.cmyk.m;
	value.cmyk.y += other.value.cmyk.y;
	value.cmyk.k += other.value.cmyk.k;
	break;
      case YUV:
	value.yuv.y += other.value.yuv.y;
	value.yuv.u += other.value.yuv.u;
	value.yuv.v += other.value.yuv.v;
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
      case CMYK:
	value.cmyk.c -= other.value.cmyk.c;
	value.cmyk.m -= other.value.cmyk.m;
	value.cmyk.y -= other.value.cmyk.y;
	value.cmyk.k -= other.value.cmyk.k;
	break;
      case YUV:
	value.yuv.y -= other.value.yuv.y;
	value.yuv.u -= other.value.yuv.u;
	value.yuv.v -= other.value.yuv.v;
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
      case CMYK:
	value.cmyk.c *= v;
	value.cmyk.m *= v;
	value.cmyk.y *= v;
	value.cmyk.k *= v;
	break;
      case YUV:
	value.yuv.y *= v;
	value.yuv.u *= v;
	value.yuv.v *= v;
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
      case CMYK:
	value.cmyk.c /= v;
	value.cmyk.m /= v;
	value.cmyk.y /= v;
	value.cmyk.k /= v;
	break;
      case YUV:
	value.yuv.y /= v;
	value.yuv.u /= v;
	value.yuv.v /= v;
	break;
      }
      return *this;
    }
    
    //prefix
    inline iterator& operator++ () {
      switch (type) {
      case BILEVEL:
	--bitpos; ++_x;
	if (bitpos < 0 || _x == width) {
	  bitpos = 7;
	  if (_x == width)
	    _x = 0;
	  ptr = (value_t*) ((u_int8_t*) ptr + 1);
	}
	break;
      case GRAY:
	ptr = (value_t*) ((u_int8_t*) ptr + 1); break;
      case GRAY16:
	ptr = (value_t*) ((u_int8_t*) ptr + 2); break;
      case RGB:
      case YUV:
	ptr = (value_t*) ((u_int8_t*) ptr + 3); break;
      case RGB16:
	ptr = (value_t*) ((u_int8_t*) ptr + 6); break;
      case CMYK:
	ptr = (value_t*) ((u_int8_t*) ptr + 4); break;
      }
      return *this;
    }
    
    inline iterator& operator-- () {
      switch (type) {
      case BILEVEL:
	++bitpos;
	if (bitpos > 7) {
	  bitpos = 0;
	  ptr = (value_t*) ((u_int8_t*) ptr - 1);
	}
	break;
      case GRAY:
	ptr = (value_t*) ((u_int8_t*) ptr - 1); break;
      case GRAY16:
	ptr = (value_t*) ((u_int8_t*) ptr - 2); break;
      case RGB:
      case YUV:
	ptr = (value_t*) ((u_int8_t*) ptr - 3); break;
      case RGB16:
	ptr = (value_t*) ((u_int8_t*) ptr - 6); break;
      case CMYK:
	ptr = (value_t*) ((u_int8_t*) ptr - 4); break;
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
	ptr->gray |= (other.value.gray >> 7) << bitpos;
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
      case CMYK:
	ptr->cmyk.c = other.value.cmyk.c;
	ptr->cmyk.m = other.value.cmyk.m;
	ptr->cmyk.y = other.value.cmyk.y;
	ptr->cmyk.k = other.value.cmyk.k;
	break;
      case YUV:
	ptr->yuv.y = other.value.yuv.y;
	ptr->yuv.u = other.value.yuv.u;
	ptr->yuv.v = other.value.yuv.v;
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
