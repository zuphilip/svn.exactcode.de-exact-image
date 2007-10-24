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

        PixelIterator(const Image* img, threshold_t threshold = 0, size_t it_size = 6) :
            img(img),
            img_it(it_size),
            threshold(threshold),
            x(0),
            y(0)
        {
            // FIXME insert an optimized code path for img->h >= 3
            for (uint i = 0; i < img_it.size(); ++i) {
                img_it[i] = img->begin().at(0,std::min((int)i,img->h));
                *img_it[i];
            }
        }

        virtual ~PixelIterator() {};

        self_t& operator++()
        {
            if ( x < img->w-1 ) {
                ++x;
                for (uint i = 0; i < img_it.size(); ++i) {
                    ++img_it[i];
                    *img_it[i];
                }
            } else {
                x = 0;
                // FIXME insert a optimized code path for img->h >= y+3
                y += img_it.size();
                for (uint i = 0; i < img_it.size(); ++i) {
                    img_it[i] = img_it[i].at(x,std::min(y+(int)i,img->h));
                    *img_it[i];
                }
            }
            return *this;
        };

        const value_type operator*() const
        {
            double tmp=0;
            for (uint i = 0; i < img_it.size(); ++i) {
                tmp += img_it[i].getL();
            }
            return (tmp/img_it.size()) < threshold;
        }
            
        //value_type* operator->();
        //const value_type* operator->() const;

        self_t at(pos_t x, pos_t y) const
        {
            // FIXME insert a optimized code path for img->h >= y+3
            self_t tmp = *this;
            for (uint i = 0; i < img_it.size(); ++i) {
                tmp.img_it[i] = tmp.img_it[i].at(x,std::min(y+(int)i,img->h));
            }
            return tmp;
        }

        pos_t get_x() const { return x; }
        pos_t get_y() const { return y; }

        threshold_t get_threshold() const { return threshold; }
        void set_threshold(threshold_t new_threshold) { threshold = new_threshold; }

        bool end() const { return !(img_it[img_it.size()-1] != img->end()); }

    protected:
        const Image* img;
        std::vector<Image::const_iterator> img_it;
        threshold_t threshold;
        pos_t x;
        pos_t y;

    }; // class PixelIterator

}; // namespace BarDecode

#endif // _BAR_DECODE_PIXEL_ITERATOR_HH_
