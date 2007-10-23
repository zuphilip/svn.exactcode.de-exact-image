#ifndef _BARDECODE_SCANNER_HH_
#define _BARDECODE_SCANNER_HH_

#include <map>

#include "BarDecodeTokenizer.hh"

namespace BarDecode
{

    // 16 bit should be enough we do not want to use all of these anyway.
    typedef uint16_t module_word_t;

    typedef uint32_t code_type_t; // Bitset of types

    typedef std::pair<char,bool> symbol_t; // where bool is the parity

    typedef std::vector<bar_t> bar_vector_t;

    enum {
        ean8 = 1,
        ean13 = 2,
        upca = 4,
        upce = 8,
    };

    // 0 means no match
    class translation_table_t
    {
    private:
        typedef std::map<module_word_t,symbol_t> map_t;

    public:
        translation_table_t(code_type_t);
        translation_table_t();

        virtual ~translation_table_t() {};

        symbol_t lookup(module_word_t mword) const
        {
            map_t::const_iterator tmp = map.find(mword);
            if (tmp != map.end()) {
                return tmp->second;
            } else {
                return symbol_t(0,0);
            }
        }

    private:
        map_t& map;
    };

    struct scanner_result_t
    {
        bool valid;
        std::string code;

        operator bool() const
        {
            return valid;
        }
    };

    extern char* ean, eanaux, ean13_0;
    enum {
        ean_normal_guard,
        ean_center_guard,
        ean_special_guard,
        ean_add_on_guard,
        ean_add_on_delineator
    };


    class Scanner
    {
    public:

        // modulizer points to first (black) module
        Scanner(Modulizer modulizer)
            : modulizer(modulizer)
        {
        }

        scanner_result_t operator()()
        {
            // try to match start marker
            
            
            // adjust code_type
            // update code_type of modulizer (TODO)
            // Set parameter (most of all module_word_size)

            // scan modules according to code_type
            return scanner_result_t();
        }

        static module_word_t get_module_word(const bar_vector_t& v);
        static bool get_parity(const module_word_t& w);
        static bool get_parity(const bar_vector_t& w);

    protected:
        Modulizer modulizer;
        code_type_t type;
        translation_table_t table;
    };


    class BarCodeIterator
    {
    public:
        typedef BarCodeIterator self_t;
        typedef std::string value_type;

        BarCodeIterator(const Image* img, threshold_t threshold) :
            img(img),
            mit(img,threshold)
        {}

        virtual ~BarCodeIterator() {}

        self_t& operator++()
        {
            assert(! end());
            do {
                ++mit;
                if (mit.end()) break;
                Scanner scanner(*mit);
                cur = scanner();
            } while (! cur.valid);
        }

        value_type operator*() const
        {
            assert(! end());
            return cur.code;

        }

        bool end() const
        {
            return mit.end();
        }

    private:
        const Image* img;
        ModulizerIterator mit;
        scanner_result_t cur;
    };


}; // namespace BarDecode

#endif // _BARDECODE_SCANNER_HH_
