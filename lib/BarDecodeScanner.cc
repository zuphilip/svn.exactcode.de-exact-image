#include "BarDecodeScanner.hh"

#define PUT_IN_TABLE(a,b,c) \
    a[b] = c; \

#define DECLARE_TABLE(a,s) \
    char a[s];

#define INIT_TABLE(a,s) \
    for(unsigned int i = 0; i < s; ++i) { \
        a[i] = 0; \
    }

#define FLIP(a,mask) (~a)&mask

namespace BarDecode
{
    uint Scanner::modules_count(const bar_vector_t& v)
    {
        uint result = 0;
        for (uint i = 0; i < v.size(); ++i) {
            result += v[i].second;
        }
        return result;
    }

    // translates a sequence of bars into an module_word (uint16_t)
    // for efficient lookup in c array;
    module_word_t Scanner::get_module_word(const bar_vector_t& v)
    {
        assert(modules_count(v) <= 16);
        module_word_t tmp = 0;
        uint s = v.size();
        for(uint i = 0 ; i < s; ++i) {
            assert(v[i].second < 5 && v[i].second > 0); // at most 2 bit
            tmp <<= v[i].second;
            if (v[i].first) {
                switch (v[i].second) {
                case 1: tmp |= 0x1; break;
                case 2: tmp |= 0x3; break;
                case 3: tmp |= 0x7; break;
                case 4: tmp |= 0xF; break;
                default: assert(false);
                }
            }
        }
        return tmp;
    }

    bool Scanner::get_parity(const module_word_t& w)
    {
        unsigned int tmp = 0;
        module_word_t x = w;
        while ( x != 0 ) { tmp += 1 & x; x >>= 1; }
        return 1 & tmp; // return parity bit
    }

    bool Scanner::get_parity(const bar_vector_t& v)
    {
        unsigned int tmp = 0;
        for (uint i = 0; i < v.size(); ++i) {
            if (v[i].first) tmp += v[i].second;
        }
        return 1 & tmp; //  return parity bit
    }


    namespace {

    struct tables_t
    {

        tables_t() 
        {
            init_tables();
        }

        DECLARE_TABLE(ean,128);
        DECLARE_TABLE(ean13_0,64);
        DECLARE_TABLE(eanaux,32);

        void init_tables()
        {

            // EAN Tables (table A,B,C are put into on array) (5.1.1.2.1)
            INIT_TABLE(ean,128);

            // EAN Table A (parity odd)
            PUT_IN_TABLE(ean,0x0D,'0');
            PUT_IN_TABLE(ean,0x19,'1');
            PUT_IN_TABLE(ean,0x13,'2');
            PUT_IN_TABLE(ean,0x3D,'3');
            PUT_IN_TABLE(ean,0x23,'4');
            PUT_IN_TABLE(ean,0x31,'5');
            PUT_IN_TABLE(ean,0x2F,'6');
            PUT_IN_TABLE(ean,0x3B,'7');
            PUT_IN_TABLE(ean,0x37,'8');
            PUT_IN_TABLE(ean,0x0B,'9');

            // EAN Table B (parity even)
#define EANB(a) (0x40&(((~a)&127)<<6)) | \
                (0x20&(((~a)&127)<<4)) | \
                (0x10&(((~a)&127)<<2)) | \
                (0x01&(((~a)&127)>>6)) | \
                (0x02&(((~a)&127)>>4)) | \
                (0x04&(((~a)&127)>>2)) | \
                (0x08&((~a)&127))
            // mirror of EANC
            PUT_IN_TABLE(ean,EANB(0x0D),'0');
            PUT_IN_TABLE(ean,EANB(0x19),'1');
            PUT_IN_TABLE(ean,EANB(0x13),'2');
            PUT_IN_TABLE(ean,EANB(0x3D),'3');
            PUT_IN_TABLE(ean,EANB(0x23),'4');
            PUT_IN_TABLE(ean,EANB(0x31),'5');
            PUT_IN_TABLE(ean,EANB(0x2F),'6');
            PUT_IN_TABLE(ean,EANB(0x3B),'7');
            PUT_IN_TABLE(ean,EANB(0x37),'8');
            PUT_IN_TABLE(ean,EANB(0x0B),'9');

            // EAN Table C (parity even)
#define EANC(a) (~a)&127  // bit complement of A (7 bit)
            PUT_IN_TABLE(ean,EANC(0x0D),'0');
            PUT_IN_TABLE(ean,EANC(0x19),'1');
            PUT_IN_TABLE(ean,EANC(0x13),'2');
            PUT_IN_TABLE(ean,EANC(0x3D),'3');
            PUT_IN_TABLE(ean,EANC(0x23),'4');
            PUT_IN_TABLE(ean,EANC(0x31),'5');
            PUT_IN_TABLE(ean,EANC(0x2F),'6');
            PUT_IN_TABLE(ean,EANC(0x3B),'7');
            PUT_IN_TABLE(ean,EANC(0x37),'8');
            PUT_IN_TABLE(ean,EANC(0x0B),'9');

            // EAN Auxiliary Pattern Table (5.1.1.2.2)
            INIT_TABLE(eanaux,32);
            PUT_IN_TABLE(eanaux,0x05,ean_normal_guard); // normal guard pattern, 3 modules
            PUT_IN_TABLE(eanaux,0x0A,ean_center_guard); // center guard pattern, 5 modules
            PUT_IN_TABLE(eanaux,0x15,ean_special_guard); // special guard pattern, 6 modules
            PUT_IN_TABLE(eanaux,0x0B,ean_add_on_guard); // add-on guard pattern, 4 modules
            PUT_IN_TABLE(eanaux,0x01,ean_add_on_delineator); // add-on delineator, 2 modules

            INIT_TABLE(ean13_0,64);
            PUT_IN_TABLE(ean13_0,0x3f,'0');
            PUT_IN_TABLE(ean13_0,0x34,'1');
            PUT_IN_TABLE(ean13_0,0x32,'2');
            PUT_IN_TABLE(ean13_0,0x31,'3');
            PUT_IN_TABLE(ean13_0,0x2c,'4');
            PUT_IN_TABLE(ean13_0,0x26,'5');
            PUT_IN_TABLE(ean13_0,0x23,'6');
            PUT_IN_TABLE(ean13_0,0x2a,'7');
            PUT_IN_TABLE(ean13_0,0x29,'8');
            PUT_IN_TABLE(ean13_0,0x25,'9');
        }
    } tables;

    }; // annonymous namespace

    // TODO if sucessfull: Advance ModulizerIterator behind right quiet zone.
    //      (and skip this bar code in the future/ check for bottom quiet zone)

    scanner_result_t Scanner::operator()()
    {
        // get x and y
        pos_t x = modulizer.get_x();
        pos_t y = modulizer.get_y();

        // try to match start marker

        // try ean with 3 bars
        bar_vector_t b(3);
        if ( get_bars(b,3) != 3) return scanner_result_t();

        module_word_t mw = get_module_word(b);
        char result = tables.eanaux[mw];
        std::string code = "";

        if (result == ean_normal_guard) {

#define SCANNER_DEBUG
#ifdef SCANNER_DEBUG
            std::cerr << "##### ean: (" << std::dec << x << ", " << y << "), unit=" 
                << modulizer.get_unit() << ", quiet=" << modulizer.get_quiet() << " : " 
                << std::hex << mw << " --> " << (int) result << std::endl;
#endif

            uint bps = 4; // (bars per symbol) a symbol has 4 bars
            module_word_t parities = 0;
            uint symbols_count = 0;
            char result = 0;
            bar_vector_t b(bps);
            uint mc = 0;
            do {
                // get symbol
                if ( get_bars(b,4) != 4 ) return scanner_result_t();
                mw = get_module_word(b);
                mc = modules_count(b);
                if (mc == 7) {
                    result = tables.ean[mw];
                    if (! result) return scanner_result_t();
                    ++symbols_count;
                    parities <<= 1;
                    parities |= get_parity(mw);
                    code += result;
#ifdef SCANNER_DEBUG
                    std::cerr << "symbol: '" << result << "'" << std::endl;
#endif
                }

            } while ( mc == 7 );

            if ( ! symbols_count ) return scanner_result_t();

            if (mc != 4 ||
                add_bars(b,1) != 1) return scanner_result_t();

            mw = get_module_word(b);
            if (tables.eanaux[mw] != ean_center_guard) return scanner_result_t();

            // TODO check for special guard (we need to implement add_bar() method

            for (uint i = 0; i < symbols_count; ++i) {

                if ( get_bars(b,4) != 4 ) return scanner_result_t();
                module_word_t mw = get_module_word(b);
                uint mc = modules_count(b);
                if (mc == 7) {
                    result = tables.ean[mw];
                    if (! result) return scanner_result_t();
                    code += result;

#ifdef SCANNER_DEBUG
                    std::cerr << "symbol: '" << result << "'" << std::endl;
#endif
                } else {
                    return scanner_result_t();
                }
            }

            // expect normal guard
            if ( get_bars(b,3) != 3) return scanner_result_t();
            module_word_t mw = get_module_word(b);
            result = tables.eanaux[mw];
            if (result == ean_normal_guard) {

                // TODO check right quiet zone

            } else if (result == ean_add_on_guard) {

                // TODO
                assert(false);

            } else {
                return scanner_result_t();
            }

            // for ean13: lookup bit 0
            if (symbols_count == 6) {
                result = tables.ean13_0[parities];
                if (! result) return scanner_result_t();
                std::string tmp = "";
                tmp.push_back(result);
                code = tmp + code;
            }

            // adjust code_type
            // update code_type of modulizer (TODO)
            // Set parameter (most of all module_word_size)

            // scan modules according to code_type
            return scanner_result_t(code,x,y);
        }

        return scanner_result_t();

    }

};
