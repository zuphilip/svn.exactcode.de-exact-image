#ifndef _BAR_DECODE_PIXEL_ITERATOR_HH_
#define _BAR_DECODE_PIXEL_ITERATOR_HH_

#include <iterator>

#include "Image.hh"

namespace BarDecode
{

    typedef int pos_t;
    typedef int threshold_t;

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
            img_it(img->begin()),
            threshold(threshold),
            x(0),
            y(0)
        {}

        virtual ~PixelIterator() {};

        self_t& operator++()
        {
            if ( x < img->w ) {
                ++x;
            } else {
                x = 0;
                ++y;
            }
            ++img_it;
            *img_it;
            return *this;
        };

        const value_type operator*() const
        {
            return (uint16_t) img_it.getL() < (uint16_t) threshold;
        }
            
        //value_type* operator->();
        //const value_type* operator->() const;

        self_t at(pos_t x, pos_t y) const
        {
            self_t tmp = *this;
            tmp.img_it.at(x,y);
            return tmp;
        }

        pos_t get_x() const { return x; }
        pos_t get_y() const { return y; }

        threshold_t get_threshold() const { return threshold; }
        void set_threshold(threshold_t new_threshold) { threshold = new_threshold; }

        bool end() const { return !(img_it != img->end()); }

    protected:
        const Image* img;
        Image::iterator img_it;
        threshold_t threshold;
        pos_t x;
        pos_t y;

    }; // class PixelIterator

}; // namespace BarDecode

#endif // _BAR_DECODE_PIXEL_ITERATOR_HH_
