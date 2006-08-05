
#ifndef IMAGE_HH
#define IMAGE_HH

#include <inttypes.h>
#include <string>

class Image
{
public:
  
  typedef enum {
    GRAY1,
    GRAY2,
    GRAY4,
    GRAY8,
    GRAY16,
    RGB8,
    RGB16,
    CMYK8,
    YUV8
  } type_t;

  typedef union {
    uint8_t gray;
    uint16_t gray16;
    struct {
      uint8_t r;
      uint8_t g;
      uint8_t b;
    } rgb;
    struct {
      uint16_t r;
      uint16_t g;
      uint16_t b;
    } rgb16;
    struct {
      uint8_t c;
      uint8_t m;
      uint8_t y;
      uint8_t k;
    } cmyk;
    struct {
      uint8_t y;
      uint8_t u;
      uint8_t v;
    } yuv;
  } value_t;
    
  typedef union {
    int32_t gray;
    struct {
      int32_t r;
      int32_t g;
      int32_t b;
    } rgb;
    struct {
      int32_t c;
      int32_t m;
      int32_t y;
      int32_t k;
    } cmyk;
    struct {
      int32_t y;
      int32_t u;
      int32_t v;
    } yuv;
  } ivalue_t;

  int w, h, bps, spp, xres, yres;
  unsigned char* data;
  
  // quick "hack" to let --density save untouched jpeg coefficients
  void* priv_data;
  bool priv_data_valid;
  
  Image ()
    : data(0), priv_data(0), priv_data_valid(false) {
  }
  
  ~Image () {
    if (data)
      free (data);
  }
  
  Image& operator= (Image& other)
  {
    w = other.w;
    h = other.h;
    bps = other.bps;
    spp = other.spp;
    xres = other.xres;
    yres = other.yres;
    if (data)
      free (data);
    data = other.data;
    other.data = 0;
    return *this;
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
    case 1:  return GRAY1;
    case 2:  return GRAY2;
    case 4:  return GRAY4;
    case 8:  return GRAY8;
    case 16: return GRAY16;
    case 24: return RGB8;
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
	ptr = (value_t*) (image->data + stride * image->h);
	_x = width;
	// TODO: bitpos= ...
      }
    }
    
    inline void clear () {
      switch (type) {
      case GRAY1:
      case GRAY2:
      case GRAY4:
      case GRAY8:
      case GRAY16:
	value.gray = 0;
	break;
      case RGB8:
      case RGB16:
	value.rgb.r = value.rgb.g = value.rgb.b = 0;
	break;
      case CMYK8:
	value.cmyk.c = value.cmyk.m = value.cmyk.y = value.cmyk.k = 0;
	break;
      case YUV8:
	value.yuv.y = value.yuv.u = value.yuv.v = 0;
	break;
      }
    }
    
    inline iterator at (int x, int y) {
      iterator tmp = *this;
      
      switch (type) {
      case GRAY1:
	tmp.ptr = (value_t*) (image->data + stride * y + x / 8);
	tmp.bitpos = 7 - x % 8;
	break;
      case GRAY2:
	tmp.ptr = (value_t*) (image->data + stride * y + x / 4);
	tmp.bitpos = 7 - (x % 4) * 2;
	break;
      case GRAY4:
	tmp.ptr = (value_t*) (image->data + stride * y + x / 2);
	tmp.bitpos = 7 - (x % 2) * 4;
	break;
      case GRAY8:
	tmp.ptr = (value_t*) (image->data + stride * y + x);
	break;
      case GRAY16:
	tmp.ptr = (value_t*) (image->data + stride * y + x * 2);
	break;
      case RGB8:
      case YUV8:
	tmp.ptr = (value_t*) (image->data + stride * y + x * 3);
	break;
      case RGB16:
	tmp.ptr = (value_t*) (image->data + stride * y + x * 6);
	break;
      case CMYK8:
	tmp.ptr = (value_t*) (image->data + stride * y + x * 4);
	break;
      }
      return tmp;
    }

    inline iterator& operator* () {
      switch (type) {
      case GRAY1:
	value.gray = (ptr->gray >> (bitpos-0) & 0x01) * 255;
	break;
      case GRAY2:
	value.gray = (ptr->gray >> (bitpos-1) & 0x03) * 255/3;
	break;
      case GRAY4:
	value.gray = (ptr->gray >> (bitpos-3) & 0x0f) * 255/15;
	break;
      case GRAY8:
	value.gray = ptr->gray;
	break;
      case GRAY16:
	value.gray = ptr->gray16;
	break;
      case RGB8:
	value.rgb.r = ptr->rgb.r;
	value.rgb.g = ptr->rgb.g;
	value.rgb.b = ptr->rgb.b;
	break;
      case RGB16:
	value.rgb.r = ptr->rgb16.r;
	value.rgb.g = ptr->rgb16.g;
	value.rgb.b = ptr->rgb16.b;
	break;
      case CMYK8:
	value.cmyk.c = ptr->cmyk.c;
	value.cmyk.m = ptr->cmyk.m;
	value.cmyk.y = ptr->cmyk.y;
	value.cmyk.k = ptr->cmyk.k;
	break;
      case YUV8:
	value.yuv.y = ptr->yuv.y;
	value.yuv.u = ptr->yuv.u;
	value.yuv.v = ptr->yuv.v;
	break;
      }
      return *this;
    }
    
    inline iterator& operator+= (const iterator& other) {
      switch (type) {
      case GRAY1:
      case GRAY2:
      case GRAY4:
      case GRAY8:
      case GRAY16:
	value.gray += other.value.gray;
	break;
      case RGB8:
      case RGB16:
	value.rgb.r += other.value.rgb.r;
	value.rgb.g += other.value.rgb.g;
	value.rgb.b += other.value.rgb.b;
	break;
      case CMYK8:
	value.cmyk.c += other.value.cmyk.c;
	value.cmyk.m += other.value.cmyk.m;
	value.cmyk.y += other.value.cmyk.y;
	value.cmyk.k += other.value.cmyk.k;
	break;
      case YUV8:
	value.yuv.y += other.value.yuv.y;
	value.yuv.u += other.value.yuv.u;
	value.yuv.v += other.value.yuv.v;
	break;     
      }
      return *this;
    }
    
    inline iterator operator+ (const iterator& other) const {
      iterator tmp = *this;
      return tmp += other;
    }
    
    inline iterator& operator-= (const iterator& other)  {
      switch (type) {
      case GRAY1:
      case GRAY2:
      case GRAY4:
      case GRAY8:
      case GRAY16:
	value.gray -= other.value.gray;
	break;
      case RGB8:
      case RGB16:
	value.rgb.r -= other.value.rgb.r;
	value.rgb.g -= other.value.rgb.g;
	value.rgb.b -= other.value.rgb.b;
	break;
      case CMYK8:
	value.cmyk.c -= other.value.cmyk.c;
	value.cmyk.m -= other.value.cmyk.m;
	value.cmyk.y -= other.value.cmyk.y;
	value.cmyk.k -= other.value.cmyk.k;
	break;
      case YUV8:
	value.yuv.y -= other.value.yuv.y;
	value.yuv.u -= other.value.yuv.u;
	value.yuv.v -= other.value.yuv.v;
	break;
      }
      return *this;
    }
    
    inline iterator& operator- (const iterator& other) const {
      iterator tmp = *this;
      return tmp -= other;
    }
    
    inline iterator& operator*= (const int v) {
      switch (type) {
      case GRAY1:
      case GRAY2:
      case GRAY4:
      case GRAY8:
      case GRAY16:
	value.gray *= v;
	break;
      case RGB8:
      case RGB16:
	value.rgb.r *= v;
	value.rgb.g *= v;
	value.rgb.b *= v;
	break;
      case CMYK8:
	value.cmyk.c *= v;
	value.cmyk.m *= v;
	value.cmyk.y *= v;
	value.cmyk.k *= v;
	break;
      case YUV8:
	value.yuv.y *= v;
	value.yuv.u *= v;
	value.yuv.v *= v;
	break;
      }
      return *this;
    }
    
    inline iterator operator* (const int v) const {
      iterator tmp = *this;
      return tmp *= v;
    }
    
    inline iterator& operator/= (const int v) {
      switch (type) {
      case GRAY1:
      case GRAY2:
      case GRAY4:
      case GRAY8:
      case GRAY16:
	value.gray /= v;
	break;
      case RGB8:
      case RGB16:
	value.rgb.r /= v;
	value.rgb.g /= v;
	value.rgb.b /= v;
	break;
      case CMYK8:
	value.cmyk.c /= v;
	value.cmyk.m /= v;
	value.cmyk.y /= v;
	value.cmyk.k /= v;
	break;
      case YUV8:
	value.yuv.y /= v;
	value.yuv.u /= v;
	value.yuv.v /= v;
	break;
      }
      return *this;
    }

    inline iterator& operator/ (const int v) const {
      iterator tmp = *this;
      return tmp /= v;
    }    
    
    //prefix
    inline iterator& operator++ () {
      switch (type) {
      case GRAY1:
	--bitpos; ++_x;
	if (bitpos < 0 || _x == width) {
	  bitpos = 7;
	  if (_x == width)
	    _x = 0;
	  ptr = (value_t*) ((uint8_t*) ptr + 1);
	}
	break;
      case GRAY2:
	bitpos -= 2; ++_x;
	if (bitpos < 0 || _x == width) {
	  bitpos = 7;
	  if (_x == width)
	    _x = 0;
	  ptr = (value_t*) ((uint8_t*) ptr + 1);
	}
	break;
      case GRAY4:
	bitpos -= 4; ++_x;
	if (bitpos < 0 || _x == width) {
	  bitpos = 7;
	  if (_x == width)
	    _x = 0;
	  ptr = (value_t*) ((uint8_t*) ptr + 1);
	}
	break;
      case GRAY8:
	ptr = (value_t*) ((uint8_t*) ptr + 1);
	break;
      case GRAY16:
	ptr = (value_t*) ((uint8_t*) ptr + 2); break;
      case RGB8:
      case YUV8:
	ptr = (value_t*) ((uint8_t*) ptr + 3); break;
      case RGB16:
	ptr = (value_t*) ((uint8_t*) ptr + 6); break;
      case CMYK8:
	ptr = (value_t*) ((uint8_t*) ptr + 4); break;
      }
      return *this;
    }
    
    inline iterator& operator-- () {
      switch (type) {
      case GRAY1:
	++bitpos; --_x;
	if (bitpos > 7) {
	  bitpos = 0;
	  ptr = (value_t*) ((uint8_t*) ptr - 1);
	}
	break;
      case GRAY2:
	bitpos += 2; --_x;
	if (bitpos > 7) {
	  bitpos = 1;
	  ptr = (value_t*) ((uint8_t*) ptr - 1);
	}
	break;
      case GRAY4:
	bitpos += 4; --_x;
	if (bitpos > 7) {
	  bitpos = 3;
	  ptr = (value_t*) ((uint8_t*) ptr - 1);
	}
	break;
      case GRAY8:
	ptr = (value_t*) ((uint8_t*) ptr - 1); break;
      case GRAY16:
	ptr = (value_t*) ((uint8_t*) ptr - 2); break;
      case RGB8:
      case YUV8:
	ptr = (value_t*) ((uint8_t*) ptr - 3); break;
      case RGB16:
	ptr = (value_t*) ((uint8_t*) ptr - 6); break;
      case CMYK8:
	ptr = (value_t*) ((uint8_t*) ptr - 4); break;
      }
      return *this;
    }
    
    // return Luminance
    inline uint16_t getL ()
    {
      switch (type) {
      case GRAY1:
      case GRAY2:
      case GRAY4:
      case GRAY8:
      case GRAY16:
	return value.gray;
	break;
      case RGB8:
      case RGB16:
	return (uint16_t) (.21267 * value.rgb.r +
			    .71516 * value.rgb.g +
			    .07217 * value.rgb.b);
	break;
      case CMYK8:
	return value.cmyk.k; // TODO
	break;
      case YUV8:
	return value.yuv.y;
	break;
      }
    }
    
    // return Luminance
    inline void getRGB(uint16_t* r, uint16_t* g, uint16_t* b)
    {
      switch (type) {
      case GRAY1:
      case GRAY2:
      case GRAY4:
      case GRAY8:
      case GRAY16:
	*r = *g = *b = value.gray;
	return;
	break;
      case RGB8:
      case RGB16:
	*r = value.rgb.r;
	*g = value.rgb.g;
	*b = value.rgb.b;
	return;
	break;
      case CMYK8:
	// TODO
	break;
      case YUV8:
	// TODO
	break;
      }
    }
    
    // return Luminance
    inline void setL (uint16_t L)
    {
      switch (type) {
      case GRAY1:
      case GRAY2:
      case GRAY4:
      case GRAY8:
      case GRAY16:
	value.gray = L;
	break;
      case RGB8:
      case RGB16:
	value.rgb.r = value.rgb.g = value.rgb.b = L;
	break;
      case CMYK8:
	// TODO:
	value.cmyk.c = value.cmyk.m = value.cmyk.y = 0;
	value.cmyk.k = L;
	break;
      case YUV8:
	value.yuv.u = value.yuv.v = 0;
	value.yuv.y = L;
 	break;
      }
    }
    inline void set (const iterator& other) {
      switch (type) {
      case GRAY1:
	ptr->gray |= (other.value.gray >> 7) << bitpos;
	break;
      case GRAY2:
	ptr->gray |= (other.value.gray >> 6) << (bitpos-1);
	break;
      case GRAY4:
	ptr->gray |= (other.value.gray >> 4) << (bitpos-3);
	break;
      case GRAY8:
	ptr->gray = other.value.gray;
	break;
      case GRAY16:
	ptr->gray16 = other.value.gray;
	break;
      case RGB8:
	ptr->rgb.r = other.value.rgb.r;
	ptr->rgb.g = other.value.rgb.g;
	ptr->rgb.b = other.value.rgb.b;
	break;
      case RGB16:
	ptr->rgb16.r = other.value.rgb.r;
	ptr->rgb16.g = other.value.rgb.g;
	ptr->rgb16.b = other.value.rgb.b;
	break;
      case CMYK8:
	ptr->cmyk.c = other.value.cmyk.c;
	ptr->cmyk.m = other.value.cmyk.m;
	ptr->cmyk.y = other.value.cmyk.y;
	ptr->cmyk.k = other.value.cmyk.k;
	break;
      case YUV8:
	ptr->yuv.y = other.value.yuv.y;
	ptr->yuv.u = other.value.yuv.u;
	ptr->yuv.v = other.value.yuv.v;
	break;
      }
    }
    
    bool operator != (const iterator& other)
    {
      switch (type) {
      case GRAY1:
      case GRAY2:
      case GRAY4:
	return ptr != other.ptr && _x != other._x;
      case GRAY8:
      case GRAY16:
      case RGB8:
      case RGB16:
      case CMYK8:
      case YUV8:
	return ptr != other.ptr;
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
