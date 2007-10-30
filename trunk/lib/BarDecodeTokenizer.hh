#ifndef _BARDECODE_TOKENIZER_HH_
#define _BARDECODE_TOKENIZER_HH_

#include <utility>
#include <assert.h>

#include <math.h>

#include "Image.hh"
#include "BarDecodePixelIterator.hh"

namespace BarDecode
{

    // Shall we include absolut position?
    typedef std::pair<bool,uint> token_t; // (color,length in pixels)
    typedef bool module_t;
    typedef double unit_t;

    class Tokenizer
    {
    public:
        Tokenizer(const Image* img,threshold_t threshold = 170) :
            img(img),
            it(img,threshold),
            extra(0)
        {}

        virtual ~Tokenizer() {}

        // precondition: ! end()
        // FIXME use extra only if u is small enough such that subpixel dimensions matter
        token_t next()
        {
            assert(! end());
            double count = 0;
            bool color = *it; // TODO simple alternation would safe the call of operator*
            do { 
                ++it; 
                ++count; 
            } while ( ! end() && color == *it );

            count -= extra;
            double extra = ( *it ? 
                             (it.get_lum() / 255.0) :
                             (1- (it.get_lum() / 255.0))
                           );
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

    protected:
        const Image* img;
        PixelIterator<> it;
        double extra;
    };

}; // namespace BarDecode

#endif // _BARDECODE_TOKENIZER_HH_
