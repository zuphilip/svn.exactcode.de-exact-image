#ifndef _BARDECODE_SCANNER_HH_
#define _BARDECODE_SCANNER_HH_

#include <map>
#include <vector>
#include <string>

#include "BarDecodeModulizer.hh"

namespace BarDecode
{

    // 16 bit should be enough we do not want to use all of these anyway.
    typedef uint16_t module_word_t;

    typedef uint32_t code_type_t; // Bitset of types

    typedef std::pair<char,bool> symbol_t; // where bool is the parity

    typedef std::vector<bar_t> bar_vector_t;

    enum {
        ean8 = 1<<0,
        ean13 = 1<<1,
        upca = 1<<2,
        upce = 1<<3,
    };

    // 0 means no match
    class translation_table_t
    {
    private:
        typedef std::map<module_word_t,symbol_t> map_t;

    public:
        translation_table_t(code_type_t) {};
        translation_table_t() :
            map()
        {};

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
        map_t map;
    };

    struct scanner_result_t
    {
        scanner_result_t() : 
            valid(false), 
            code(""),
            x(0),
            y(0)
        {}
        
        scanner_result_t(const std::string& code, pos_t x, pos_t y) :
            valid(true),
            code(code),
            x(x),
            y(y)
        {}

        bool valid;
        std::string code;
        pos_t x;
        pos_t y;

        operator bool() const
        {
            return valid;
        }
    };

    enum {
        ean_normal_guard = 1,
        ean_center_guard = 2,
        ean_special_guard = 3,
        ean_add_on_guard = 4,
        ean_add_on_delineator = 5
    };


    class Scanner
    {
    public:

        // modulizer points to first (black) module
        Scanner(Modulizer modulizer) :
            modulizer(modulizer),
            table()
        {

        }

        void init_tables() const;

        uint add_bars(bar_vector_t& v,uint c)
        {
            size_t old_size = v.size();
            v.resize(old_size + c);
            for (uint i = 0; i < c; ++i) {
                if (modulizer.end()) {
                    v.resize(old_size + i);
                    return i;
                } else {
                    v[old_size + i] = *modulizer;
                    ++modulizer;
                }
            }
            return c;
        }

        uint get_bars(bar_vector_t& v,uint c)
        {
            v.resize(c);
            for (uint i = 0; i < c; ++i) {
                if (modulizer.end()) {
                    v.resize(i);
                    return i;
                } else {
                    v[i] = *modulizer;
                    ++modulizer;
                }
            }
            return c;
        }

        scanner_result_t operator()();

        static uint modules_count(const bar_vector_t& v);
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
        {
            if (! mit.end()) {
                operator++();
            }
        }

        virtual ~BarCodeIterator() {}

        self_t& operator++()
        {
            assert(! end());
            do {
                ++mit;
                if (mit.end()) break;

                Modulizer ml = *mit, mu = *mit;
                Scanner scanner(*mit);
                cur = scanner();

                if ( ! cur ) { 
                    mu.set_unit( ml.get_unit() * 1.25);
                    cur = (Scanner(mu))();
                    if ( ! cur ) { 
                        ml.set_unit( ml.get_unit() * 0.75);
                        cur = (Scanner(ml))();
                    }
                }

            } while (! cur );
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
