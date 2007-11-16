#ifndef _SCANNER_HH_
#define _SCANNER_HH_

#include <map>
#include <vector>
#include <string>

#include "Tokenizer.hh"

namespace BarDecode
{

    // 16 bit should be enough we do not want to use all of these anyway.
    typedef uint16_t module_word_t;

    typedef uint32_t codes_t; // Bitset of types

    typedef std::pair<char,bool> symbol_t; // where bool is the parity

    typedef unsigned int psize_t; // size in pixel type
    typedef unsigned int usize_t; // size in X unit

    typedef double u_t; // type for X unit

    enum code_t {
        ean8 = 1<<0,
        ean13 = 1<<1,
        upca = 1<<2,
        ean = ean8|ean13|upca,
        upce = 1<<3,
        code128 = 1<<4,
        gs1_128 = 1<<5,
        code39 = 1<<6,
        code39_mod43 = 1<<7,
        code39_ext = 1<<8,
        code25i = 1<<9
    };

    std::ostream& operator<< (std::ostream& s, const code_t& t);

    struct bar_vector_t : public std::vector<token_t> 
    {
        bar_vector_t(int s) :
            std::vector<token_t>(s),
            psize(0)
        {}
        psize_t psize;
    };

    struct scanner_result_t
    {
        scanner_result_t() : 
            valid(false),
            type(),
            code(""),
            x(0),
            y(0)
        {}
        
        scanner_result_t(code_t type, const std::string& code, pos_t x, pos_t y) :
            valid(true),
            type(type),
            code(code),
            x(x),
            y(y)
        {}

        bool valid;
        code_t type;
        std::string code;
        pos_t x;
        pos_t y;

        operator bool() const
        {
            return valid;
        }
    };

    class BarcodeIterator
    {
    public:
        typedef BarcodeIterator self_t;
        typedef scanner_result_t value_type;

        static const psize_t min_quiet_psize = 7;

        BarcodeIterator(const Image* img, threshold_t threshold, codes_t requested_codes) :
            tokenizer(img,threshold),
            requested_codes(requested_codes),
            cur_barcode()
        {
            if ( ! end() ) next();
        }

        virtual ~BarcodeIterator() {}

        value_type operator*() const
        {
            return cur_barcode;
        }

        // Try to find next modulizer
        self_t& operator++()
        {
            assert(! end());
            next();
            return *this;
        }

        bool end() const { return tokenizer.end(); }

    private:

        bool requested(code_t code) const
        {
            return code & requested_codes;
        }

        // TODO considere all possibilities for unit and quiet instanciations
        void next();

    private:
        Tokenizer tokenizer;
        codes_t requested_codes;
        scanner_result_t cur_barcode;
    };

}; // namespace BarDecode

#endif // _SCANNER_HH_
