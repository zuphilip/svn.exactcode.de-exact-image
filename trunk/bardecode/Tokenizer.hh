#ifndef _TOKENIZER_HH_
#define _TOKENIZER_HH_

#include <utility>
#include <assert.h>

#include <math.h>

#include <algorithm>

#include "Image.hh"
#include "PixelIterator.hh"

namespace BarDecode
{

    // Shall we include absolut position?
    typedef std::pair<bool,uint> token_t; // (color,length in pixels)
    typedef bool module_t;
    typedef double unit_t;

    template<bool vertical = false>
    class Tokenizer
    {
    public:
        Tokenizer(const Image* img, int concurrent_lines = 4, int line_skip = 8, threshold_t threshold = 150) :
            img(img),
            it(img,concurrent_lines,line_skip,threshold),
            extra(0)
        {}

        virtual ~Tokenizer() {}

        // precondition: ! end()
        // FIXME use extra only if u is small enough such that subpixel dimensions matter
        token_t next()
        {
            assert(! end());

#define DYNAMIC_THRESHOLD
#ifdef DYNAMIC_THRESHOLD
            double sum = 0;
#endif

            double count = 0;
            bool color = *it; // TODO simple alternation would safe the call of operator*
            double lum = it.get_lum();
            do { 

#ifdef DYNAMIC_THRESHOLD
                sum += lum;
#endif

                ++count;
                ++it; 
                lum = it.get_lum();

#ifdef DYNAMIC_THRESHOLD
                double mean =  sum / count;
                if ( ! color && lum < mean - 30) {
                    it.set_threshold(lround(std::max((double)it.get_threshold(),std::min(mean - 30,220.0))));
                } else if ( color && lum > mean + 30) {
                    it.set_threshold(lround(std::min((double)it.get_threshold(),std::max(mean + 30,80.0))));
                }
#endif

            } while ( ! end() && color == *it );

            count -= extra;
            double extra = ( *it ?  (lum / 255.0) : (1- (lum / 255.0)));
            count += extra;
            extra = 1 - extra;
            return token_t(color,lround(count));
        }

        bool end() const { return it.end(); }

        threshold_t get_threshold() const { return it.get_threshold(); }
        void set_threshold(threshold_t new_threshold) 
        { 
            it.set_threshold(new_threshold); 
        }

        pos_t get_x() const { return it.get_x(); }
        pos_t get_y() const { return it.get_y(); }
        const Image* get_img() const { return img; }

        Tokenizer at(pos_t x, pos_t y) const
        {
            Tokenizer tmp = *this;
            tmp.it = it.at(x,y);
            return tmp;
        }

    protected:
        const Image* img;
        PixelIterator<vertical> it;
        double extra;
    };

}; // namespace BarDecode

#endif // _TOKENIZER_HH_
