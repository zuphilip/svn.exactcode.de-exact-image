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

    protected:
        const Image* img;
        PixelIterator it;
        double extra;
    };

#if 0
    // TODO put it in its own header ?
    // Move initialization code out of Modulizer. 
    // Assume that a Modulizer is initialized.
    // operator bool() or end() indicates end of valid module sequence.

    class Modulizer
    {
    public:
        typedef Modulizer self_t;
        typedef bar_t value_type;

        Modulizer(const Tokenizer& tokenizer, unit_t unit, unsigned int quiet) :
            tokenizer(tokenizer),
            unit(unit),
            quiet(quiet),
            cur_bar(),
            invalid(false)
        {
            operator++();      // initialize module_count
            assert(cur_bar.first); // assert cur_color == black
        }

        virtual ~Modulizer() {};

        self_t& operator++()
        {
            assert(! end());
            assert(! tokenizer.end());
            token_t t = tokenizer.next();
            cur_bar = bar_t(t.first,lround((double)t.second/unit)); // FIXME add fuzzyness

            // check if token is valid bar
            invalid = (cur_bar.second < 1 || cur_bar.second > 4);
            return *this;
        }

        value_type operator*() const
        {
            assert(! end());
            return cur_bar;
        }

        bool end() const 
        { 
            return invalid || tokenizer.end(); 
        }

        pos_t get_x() const { return tokenizer.get_x(); }
        pos_t get_y() const { return tokenizer.get_y(); }

        unit_t get_unit() const { return unit; }
        unsigned int get_quiet() const { return quiet; }

        void set_unit(unit_t u) { unit = u; }

    protected:
        Tokenizer tokenizer;
        unit_t unit;
        unsigned int quiet;
        bar_t cur_bar;
        bool invalid;
    };
#endif


}; // namespace BarDecode

#endif // _BARDECODE_TOKENIZER_HH_
