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
            img_it(),
            threshold(threshold),
            x(0),
            y(0)
        {
            // FIXME insert a optimized code path for img->h >= 3
            img_it[0] = img->begin();
            img_it[1] = img_it[0].at(0,std::min(1,img->h));
            img_it[2] = img_it[0].at(0,std::min(2,img->h));
            img_it[3] = img_it[0].at(0,std::min(3,img->h));
            *img_it[0];
            *img_it[1];
            *img_it[2];
            *img_it[3];
        }

        virtual ~PixelIterator() {};

        self_t& operator++()
        {
            if ( x < img->w-1 ) {
                ++x;
                ++img_it[0];
                ++img_it[1];
                ++img_it[2];
                ++img_it[3];
            } else {
                x = 0;
                // FIXME insert a optimized code path for img->h >= y+3
                y += 4;
                img_it[0] = img_it[0].at(x,y+0);
                img_it[1] = img_it[1].at(x,std::min(y+1,img->h));
                img_it[2] = img_it[2].at(x,std::min(y+2,img->h));
                img_it[3] = img_it[3].at(x,std::min(y+3,img->h));
            }
            *img_it[0];
            *img_it[1];
            *img_it[2];
            *img_it[3];
            return *this;
        };

        const value_type operator*() const
        {
            double mean = ((double) (img_it[0].getL() + img_it[1].getL() + img_it[2].getL() + img_it[3].getL()))/4.0;
            return mean < threshold;
        }
            
        //value_type* operator->();
        //const value_type* operator->() const;

        self_t at(pos_t x, pos_t y) const
        {
            // FIXME insert a optimized code path for img->h >= y+3
            self_t tmp = *this;
            tmp.img_it[0] = tmp.img_it[0].at(x,y);
            tmp.img_it[1] = tmp.img_it[1].at(x,std::min(y+1,img->h));
            tmp.img_it[2] = tmp.img_it[2].at(x,std::min(y+2,img->h));
            tmp.img_it[3] = tmp.img_it[3].at(x,std::min(y+3,img->h));
            return tmp;
        }

        pos_t get_x() const { return x; }
        pos_t get_y() const { return y; }

        threshold_t get_threshold() const { return threshold; }
        void set_threshold(threshold_t new_threshold) { threshold = new_threshold; }

        bool end() const { return !(img_it[3] != img->end()); }

    protected:
        const Image* img;
        Image::const_iterator img_it[4];
        threshold_t threshold;
        pos_t x;
        pos_t y;

    }; // class PixelIterator

}; // namespace BarDecode

#endif // _BAR_DECODE_PIXEL_ITERATOR_HH_
