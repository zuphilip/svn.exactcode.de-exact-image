#ifndef _BAR_DECODE_PIXEL_ITERATOR_HH_
#define _BAR_DECODE_PIXEL_ITERATOR_HH_

#define NDEBUG

#include <iterator>

#include "Image.hh"

namespace BarDecode
{

    typedef int pos_t;
    typedef int threshold_t;

    template<int it_size = 4>
    class PixelIterator : 
        public std::iterator<std::output_iterator_tag,
                             bool,
                             std::ptrdiff_t>
    {
    protected:
        typedef PixelIterator self_t;

    public:

        typedef bool value_type;

        PixelIterator(const Image* img, threshold_t threshold = 0) :
            img(img),
            img_it(it_size),
            threshold(threshold),
            x(0),
            y(0),
            lum(0),
            valid_cache(false)
        {
            // FIXME insert an optimized code path for img->h <= it_size
            for (uint i = 0; i < it_size; ++i) {
                img_it[i] = img->begin().at(0,std::min((int)i,img->h));
                *img_it[i];
            }
        }

        virtual ~PixelIterator() {};

        self_t& operator++()
        {
            valid_cache = false;
            if ( x < img->w-1 ) {
                ++x;
                for (uint i = 0; i < it_size; ++i) {
                    ++img_it[i];
                    *img_it[i];
                }
            } else {
                x = 0;
                // FIXME insert an optimized code path for img->h >= y+it_size
                y += it_size;
                for (uint i = 0; i < it_size; ++i) {
                    img_it[i] = img_it[i].at(x,std::min(y+(int)i,img->h));
                    *img_it[i];
                }
            }
            return *this;
        };

        const value_type operator*() const
        {
            if (valid_cache) return cache;
            double tmp=0;
            //uint16_t min = 255;
            //uint16_t max = 0;
            for (uint i = 0; i < it_size; ++i) {
                //min = std::min(min,img_it[i].getL());
                //max = std::max(max,img_it[i].getL());
                tmp += img_it[i].getL();
            }
            //lum = (tmp - (double) (min+max)) / (double) (it_size-2);
            lum = tmp / (double) it_size;
            cache = lum < threshold;
            valid_cache = true;
            return cache;
        }
            
        //value_type* operator->();
        //const value_type* operator->() const;

        self_t at(pos_t x, pos_t y) const
        {
            // FIXME insert an optimized code path for img->h >= y+it_size
            self_t tmp = *this;
            for (uint i = 0; i < it_size; ++i) {
                tmp.img_it[i] = tmp.img_it[i].at(x,std::min(y+(int)i,img->h));
            }
            tmp.valid_cache = false;
            return tmp;
        }

        pos_t get_x() const { return x; }
        pos_t get_y() const { return y; }

        threshold_t get_threshold() const { return threshold; }

        void set_threshold(threshold_t new_threshold) 
        {
            valid_cache = false;
            threshold = new_threshold; 
        }

        bool end() const { return !(img_it[it_size-1] != img->end()); }

        double get_lum() const 
        {
            if (! valid_cache) {
                operator*();
            }
            return lum;
        }

    protected:
        const Image* img;
        std::vector<Image::const_iterator> img_it;
        threshold_t threshold;
        pos_t x;
        pos_t y;
        mutable double lum;
        mutable bool cache;
        mutable bool valid_cache;

    }; // class PixelIterator

}; // namespace BarDecode

#endif // _BAR_DECODE_PIXEL_ITERATOR_HH_
