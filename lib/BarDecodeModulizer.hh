#ifndef _BARDECODE_TOKENIZER_HH_
#define _BARDECODE_TOKENIZER_HH_

#include <utility>
#include <assert.h>

#include "Image.hh"
#include "BarDecodePixelIterator.hh"

namespace BarDecode
{

    // Shall we include absolut position?
    typedef std::pair<bool,uint> token_t; // (color,length in pixels)
    typedef std::pair<bool,uint> bar_t;   // (color, length in modules)
    typedef bool module_t;

    class Tokenizer
    {
    public:
        Tokenizer(const Image* img,threshold_t threshold = 170) :
            img(img),
            it(img,threshold)
        {}

        virtual ~Tokenizer() {}

        // precondition: ! end()
        token_t next()
        {
            assert(! end());
            int count = 0;
            bool color = *it; // TODO simple alternation would safe the call of operator*
            do { ++it; ++count; } while ( ! end() && color == *it );
            return token_t(color,count);
        }

        bool end() const { return it.end(); }

        threshold_t get_threshold() const { return it.get_threshold(); }
        void set_threshold(threshold_t new_threshold) { it.set_threshold(new_threshold); }

        pos_t get_x() const { return it.get_x(); }
        pos_t get_y() const { return it.get_y(); }

    protected:
        const Image* img;
        PixelIterator it;
    };


    // TODO put it in its own header ?
    // Move initialization code out of Modulizer. 
    // Assume that a Modulizer is initialized.
    // operator bool() or end() indicates end of valid module sequence.

    class Modulizer
    {
    public:
        typedef Modulizer self_t;
        typedef bar_t value_type;

        Modulizer(const Tokenizer& tokenizer, unsigned int unit) :
            tokenizer(tokenizer),
            unit(unit),
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
            cur_bar = bar_t(t.first,t.second);

            // check if token is valid bar
            invalid = (cur_bar.second <= 0 || cur_bar.second > 4);
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

    protected:
        Tokenizer tokenizer;
        unsigned int unit;
        bar_t cur_bar;
        bool invalid;
    };


    class ModulizerIterator
    {
    public:
        typedef ModulizerIterator self_t;
        typedef Modulizer value_type;

        ModulizerIterator(const Image* img, threshold_t threshold) :
            tokenizer(img,threshold),
            init(tokenizer),
            quiet(0),
            unit(0),
            valid(false)
        {
            next();
        }

        virtual ~ModulizerIterator() {}

        value_type operator*() const
        {
            return Modulizer(init,unit);
        }

        // Try to find next modulizer
        self_t& operator++()
        {
            assert(valid);
            next();
            return *this;
        }

        bool end() const { return ! valid; }

    private:
        bool check_tokenizer()
        {
            if ( tokenizer.end() ) { 
                valid = false; 
                return false;
            } else {
                return true;
            }
        }

        void next()
        {
            assert(check_tokenizer());

            bool success = false;
            while ( ! success ) {

                // get next white module and interprete it as quiet-zone
                if ( ! check_tokenizer() ) return;
                token_t t = tokenizer.next();
                if (t.first) {
                    if ( ! check_tokenizer() ) return;
                    t = tokenizer.next();
                }
                int quiet = t.second;

                // remember tokenizer as init
                // (tokenizer itself is advanced below by one token, since
                //  cur position is black it is no valid quiet-zone, anyway)
                init = tokenizer;

                // get black module and compute unit from it
                if ( ! check_tokenizer() ) return;
                t = tokenizer.next();
                assert(t.first); // assert black
                unit = t.second; // ASSUME: this holds for all codes! (FIXME verify assumption)

                // check size of q
                if (quiet >= 7*unit) { // EAN-8 specifc !!! FIXME
                    success = true;
                }
              
            }
            // we have got a hit
            // TODO initialize parameters
            // TODO maintain bitfield of possible types
        }

    private:
        Tokenizer tokenizer;
        Tokenizer init;
        unsigned int quiet;
        unsigned int unit;
        bool valid;
    };


}; // namespace BarDecode

#endif // _BARDECODE_TOKENIZER_HH_
