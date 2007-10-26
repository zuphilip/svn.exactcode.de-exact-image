#include "BarDecodeScanner.hh"

//#define SCANNER_DEBUG

#define PUT_IN_TABLE(a,b,c) \
    a[b] = c; \

#define DECLARE_TABLE(a,s) \
    char a[s];

#define INIT_TABLE(a,s,v) \
    for(unsigned int i = 0; i < s; ++i) { \
        a[i] = v; \
    }

#define FLIP(a,mask) (~a)&mask

namespace BarDecode
{
    std::ostream& operator<< (std::ostream& s, const code_t& t)
    {
        switch(t) {
        case ean8: return s << "ean8";
        case ean13: return s << "ean13";
        case upca: return s << "upca";
        case ean8|ean13|upca: return s << "ean";
        case upce: return s << "upce";
        case code128: return s << "code128";
        case gs1_128: return s << "GS1-128";
        default: return s << "unknown barcode type";
        }
    }

    // TODO make all tables static (would be nice to have something like designated initializers
    // like in C99 in C++ as well...) We do not have. Hecne we need to work around.
    // possibly by encapsulating the tables into objects with constructors that
    // perform the initialization.
    struct code128_t
    {
        enum { FNC1, FNC2, FNC3, FNC4, SHIFT, CODEA, CODEB, CODEC, STARTA, STARTB, STARTC, END };
        static const char no_entry = 255;
        enum code_set_t { code_set_a, code_set_b, code_set_c };
     
        static const usize_t min_quiet_usize = 10; 
        static const usize_t min_quiet_usize_right = 10;

        code128_t();

        scanner_result_t scan(Tokenizer, psize_t) const;
        std::string decode128(code_set_t code_set, module_word_t mw) const; 
        code_set_t shift_code_set(code_set_t code_set) const;

        DECLARE_TABLE(table,512);
        DECLARE_TABLE(aaux,10);
        DECLARE_TABLE(baux,10);
        DECLARE_TABLE(caux,10);

    } code128_impl;

    code128_t::code128_t()
    {
        // Code128 (255 indicates invalid module_word)
        // Based on 11 bits, where the first bit is 1 and the last bit is 0,
        // hence only 9 bit are used for lookup.
        INIT_TABLE(table,512,no_entry);
        PUT_IN_TABLE(table,0x166, 0);
        PUT_IN_TABLE(table,0x136, 1);
        PUT_IN_TABLE(table,0x133, 2);
        PUT_IN_TABLE(table,0x4C, 3);
        PUT_IN_TABLE(table,0x46, 4);
        PUT_IN_TABLE(table,0x26, 5);
        PUT_IN_TABLE(table,0x64, 6);
        PUT_IN_TABLE(table,0x62, 7);
        PUT_IN_TABLE(table,0x32, 8);
        PUT_IN_TABLE(table,0x124, 9);
        PUT_IN_TABLE(table,0x122, 10);
        PUT_IN_TABLE(table,0x112, 11);
        PUT_IN_TABLE(table,0xCE, 12);
        PUT_IN_TABLE(table,0x6E, 13);
        PUT_IN_TABLE(table,0x67, 14);
        PUT_IN_TABLE(table,0xE6, 15);
        PUT_IN_TABLE(table,0x76, 16);
        PUT_IN_TABLE(table,0x73, 17);
        PUT_IN_TABLE(table,0x139, 18);
        PUT_IN_TABLE(table,0x12E, 19);
        PUT_IN_TABLE(table,0x127, 20);
        PUT_IN_TABLE(table,0x172, 21);
        PUT_IN_TABLE(table,0x13A, 22);
        PUT_IN_TABLE(table,0x1B7, 23);
        PUT_IN_TABLE(table,0x1A6, 24);
        PUT_IN_TABLE(table,0x196, 25);
        PUT_IN_TABLE(table,0x193, 26);
        PUT_IN_TABLE(table,0x1B2, 27);
        PUT_IN_TABLE(table,0x19A, 28);
        PUT_IN_TABLE(table,0x199, 29);
        PUT_IN_TABLE(table,0x16C, 30);
        PUT_IN_TABLE(table,0x163, 31);
        PUT_IN_TABLE(table,0x11B, 32);
        PUT_IN_TABLE(table,0x8C, 33);
        PUT_IN_TABLE(table,0x2C, 34);
        PUT_IN_TABLE(table,0x23, 35);
        PUT_IN_TABLE(table,0xC4, 36);
        PUT_IN_TABLE(table,0x34, 37);
        PUT_IN_TABLE(table,0x31, 38);
        PUT_IN_TABLE(table,0x144, 39);
        PUT_IN_TABLE(table,0x114, 40);
        PUT_IN_TABLE(table,0x111, 41);
        PUT_IN_TABLE(table,0xDC, 42);
        PUT_IN_TABLE(table,0xC7, 43);
        PUT_IN_TABLE(table,0x37, 44);
        PUT_IN_TABLE(table,0xEC, 45);
        PUT_IN_TABLE(table,0xE3, 46);
        PUT_IN_TABLE(table,0x3B, 47);
        PUT_IN_TABLE(table,0x1BB, 48);
        PUT_IN_TABLE(table,0x147, 49);
        PUT_IN_TABLE(table,0x117, 50);
        PUT_IN_TABLE(table,0x174, 51);
        PUT_IN_TABLE(table,0x171, 52);
        PUT_IN_TABLE(table,0x177, 53);
        PUT_IN_TABLE(table,0x1AC, 54);
        PUT_IN_TABLE(table,0x1A3, 55);
        PUT_IN_TABLE(table,0x18B, 56);
        PUT_IN_TABLE(table,0x1B4, 57);
        PUT_IN_TABLE(table,0x1B1, 58);
        PUT_IN_TABLE(table,0x18D, 59);
        PUT_IN_TABLE(table,0x1BD, 60);
        PUT_IN_TABLE(table,0x121, 61);
        PUT_IN_TABLE(table,0x1C5, 62);
        PUT_IN_TABLE(table,0x98, 63);
        PUT_IN_TABLE(table,0x86, 64);
        PUT_IN_TABLE(table,0x58, 65);
        PUT_IN_TABLE(table,0x43, 66);
        PUT_IN_TABLE(table,0x16, 67);
        PUT_IN_TABLE(table,0x13, 68);
        PUT_IN_TABLE(table,0xC8, 69);
        PUT_IN_TABLE(table,0xC2, 70);
        PUT_IN_TABLE(table,0x68, 71);
        PUT_IN_TABLE(table,0x61, 72);
        PUT_IN_TABLE(table,0x1A, 73);
        PUT_IN_TABLE(table,0x19, 74);
        PUT_IN_TABLE(table,0x109, 75);
        PUT_IN_TABLE(table,0x128, 76);
        PUT_IN_TABLE(table,0x1DD, 77);
        PUT_IN_TABLE(table,0x10A, 78);
        PUT_IN_TABLE(table,0x3D, 79);
        PUT_IN_TABLE(table,0x9E, 80);
        PUT_IN_TABLE(table,0x5E, 81);
        PUT_IN_TABLE(table,0x4F, 82);
        PUT_IN_TABLE(table,0xF2, 83);
        PUT_IN_TABLE(table,0x7A, 84);
        PUT_IN_TABLE(table,0x79, 85);
        PUT_IN_TABLE(table,0x1D2, 86);
        PUT_IN_TABLE(table,0x1CA, 87);
        PUT_IN_TABLE(table,0x1C9, 88);
        PUT_IN_TABLE(table,0x16F, 89);
        PUT_IN_TABLE(table,0x17B, 90);
        PUT_IN_TABLE(table,0x1DB, 91);
        PUT_IN_TABLE(table,0xBC, 92);
        PUT_IN_TABLE(table,0x8F, 93);
        PUT_IN_TABLE(table,0x2F, 94);
        PUT_IN_TABLE(table,0xF4, 95);
        PUT_IN_TABLE(table,0xF1, 96);
        PUT_IN_TABLE(table,0x1D4, 97);
        PUT_IN_TABLE(table,0x1D1, 98);
        PUT_IN_TABLE(table,0xEF, 99);
        PUT_IN_TABLE(table,0xF7, 100);
        PUT_IN_TABLE(table,0x1AF, 101);
        PUT_IN_TABLE(table,0x1D7, 102);
        PUT_IN_TABLE(table,0x142, 103);
        PUT_IN_TABLE(table,0x148, 104);
        PUT_IN_TABLE(table,0x14e, 105);
        PUT_IN_TABLE(table,0x11d, 106);

        // Range 96-105. Use offset -96
        INIT_TABLE(aaux,10,255);
        PUT_IN_TABLE(aaux,0,FNC3); // 96
        PUT_IN_TABLE(aaux,1,FNC2); // 97
        PUT_IN_TABLE(aaux,2,SHIFT); // 98
        PUT_IN_TABLE(aaux,3,CODEC); // 99
        PUT_IN_TABLE(aaux,4,CODEB); // 100
        PUT_IN_TABLE(aaux,5,FNC4); // 101
        PUT_IN_TABLE(aaux,6,FNC1); // 102
        PUT_IN_TABLE(aaux,7,STARTA); // 103
        PUT_IN_TABLE(aaux,8,STARTB); // 104
        PUT_IN_TABLE(aaux,9,STARTC); // 105

        INIT_TABLE(baux,10,255);
        PUT_IN_TABLE(baux,0,FNC3); // 96
        PUT_IN_TABLE(baux,1,FNC2); // 97
        PUT_IN_TABLE(baux,2,SHIFT); // 98
        PUT_IN_TABLE(baux,3,CODEC); // 99
        PUT_IN_TABLE(baux,4,FNC4); // 100
        PUT_IN_TABLE(baux,5,CODEA); // 101
        PUT_IN_TABLE(baux,6,FNC1); // 102
        PUT_IN_TABLE(baux,7,STARTA); // 103
        PUT_IN_TABLE(baux,8,STARTB); // 104
        PUT_IN_TABLE(baux,9,STARTC); // 105

        INIT_TABLE(caux,10,255);
        PUT_IN_TABLE(caux,0,no_entry); // 96
        PUT_IN_TABLE(caux,1,no_entry); // 97
        PUT_IN_TABLE(caux,2,no_entry); // 98
        PUT_IN_TABLE(caux,3,no_entry); // 99
        PUT_IN_TABLE(caux,4,CODEB); // 100
        PUT_IN_TABLE(caux,5,CODEA); // 101
        PUT_IN_TABLE(caux,6,FNC1); // 102
        PUT_IN_TABLE(caux,7,STARTA); // 103
        PUT_IN_TABLE(caux,8,STARTB); // 104
        PUT_IN_TABLE(caux,9,STARTC); // 105
    };

    struct ean_t
    {
        enum {
            normal_guard = 1,
            center_guard = 2,
            special_guard = 3,
            add_on_guard = 4,
            add_on_delineator = 5
        };

        static const usize_t min_quiet_usize = 7;

        ean_t();
        scanner_result_t scan(Tokenizer, psize_t);

        DECLARE_TABLE(table,128);
        DECLARE_TABLE(ean13table,64);
        DECLARE_TABLE(auxtable,32);

    } ean_impl;

    ean_t::ean_t()
    {
        // EAN Tables (table A,B,C are put into on array) (5.1.1.2.1)
        INIT_TABLE(table,128,0);

        // EAN Table A (parity odd)
        PUT_IN_TABLE(table,0x0D,'0');
        PUT_IN_TABLE(table,0x19,'1');
        PUT_IN_TABLE(table,0x13,'2');
        PUT_IN_TABLE(table,0x3D,'3');
        PUT_IN_TABLE(table,0x23,'4');
        PUT_IN_TABLE(table,0x31,'5');
        PUT_IN_TABLE(table,0x2F,'6');
        PUT_IN_TABLE(table,0x3B,'7');
        PUT_IN_TABLE(table,0x37,'8');
        PUT_IN_TABLE(table,0x0B,'9');

        // EAN Table B (parity even)
#define EANB(a) (0x40&(((~a)&127)<<6)) | \
        (0x20&(((~a)&127)<<4)) | \
        (0x10&(((~a)&127)<<2)) | \
        (0x01&(((~a)&127)>>6)) | \
        (0x02&(((~a)&127)>>4)) | \
        (0x04&(((~a)&127)>>2)) | \
        (0x08&((~a)&127))
        // mirror of EANC
        PUT_IN_TABLE(table,EANB(0x0D),'0');
        PUT_IN_TABLE(table,EANB(0x19),'1');
        PUT_IN_TABLE(table,EANB(0x13),'2');
        PUT_IN_TABLE(table,EANB(0x3D),'3');
        PUT_IN_TABLE(table,EANB(0x23),'4');
        PUT_IN_TABLE(table,EANB(0x31),'5');
        PUT_IN_TABLE(table,EANB(0x2F),'6');
        PUT_IN_TABLE(table,EANB(0x3B),'7');
        PUT_IN_TABLE(table,EANB(0x37),'8');
        PUT_IN_TABLE(table,EANB(0x0B),'9');

        // EAN Table C (parity even)
#define EANC(a) (~a)&127  // bit complement of A (7 bit)
        PUT_IN_TABLE(table,EANC(0x0D),'0');
        PUT_IN_TABLE(table,EANC(0x19),'1');
        PUT_IN_TABLE(table,EANC(0x13),'2');
        PUT_IN_TABLE(table,EANC(0x3D),'3');
        PUT_IN_TABLE(table,EANC(0x23),'4');
        PUT_IN_TABLE(table,EANC(0x31),'5');
        PUT_IN_TABLE(table,EANC(0x2F),'6');
        PUT_IN_TABLE(table,EANC(0x3B),'7');
        PUT_IN_TABLE(table,EANC(0x37),'8');
        PUT_IN_TABLE(table,EANC(0x0B),'9');

        // EAN Auxiliary Pattern Table (5.1.1.2.2)
        INIT_TABLE(auxtable,32,0);
        PUT_IN_TABLE(auxtable,0x05,normal_guard); // normal guard pattern, 3 modules
        PUT_IN_TABLE(auxtable,0x0A,center_guard); // center guard pattern, 5 modules
        PUT_IN_TABLE(auxtable,0x15,special_guard); // special guard pattern, 6 modules
        PUT_IN_TABLE(auxtable,0x0B,add_on_guard); // add-on guard pattern, 4 modules
        PUT_IN_TABLE(auxtable,0x01,add_on_delineator); // add-on delineator, 2 modules

        INIT_TABLE(ean13table,64,0);
        PUT_IN_TABLE(ean13table,0x3f,'0');
        PUT_IN_TABLE(ean13table,0x34,'1');
        PUT_IN_TABLE(ean13table,0x32,'2');
        PUT_IN_TABLE(ean13table,0x31,'3');
        PUT_IN_TABLE(ean13table,0x2c,'4');
        PUT_IN_TABLE(ean13table,0x26,'5');
        PUT_IN_TABLE(ean13table,0x23,'6');
        PUT_IN_TABLE(ean13table,0x2a,'7');
        PUT_IN_TABLE(ean13table,0x29,'8');
        PUT_IN_TABLE(ean13table,0x25,'9');

    }

    void BarcodeIterator::next()
    {
        assert( ! end());
        token_t t = tokenizer.next();

        while ( ! end() ) {

            // goto next white space of psize >= min_quiet_psize
            while (t.first || t.second < min_quiet_psize) { // while black ...
                if ( end() ) return;
                else t = tokenizer.next();
            }
            assert(! t.first); // assert white

            psize_t quiet_psize = t.second;
            //if ( end() ) return;
            //t = tokenizer.next();

            scanner_result_t result;
            // try scanning for all requested barcode types
            if ( requested(ean) && (result = ean_impl.scan(tokenizer,quiet_psize)) ) {
                // TODO set tokenizer to end of barcode
                cur_barcode = result;
                return;
            } else if (requested(code128) && (result = code128_impl.scan(tokenizer,quiet_psize))) {
                // TODO set tokenizer to end of barcode
                cur_barcode = result;
                return;
            }

            if ( end() ) return;
            t = tokenizer.next();
        }
    }    

    namespace scanner_utilities
    {
        template<class CODE>
        u_t max_u(psize_t quiet_psize, double tolerance = 0.35)
        {
            return (quiet_psize / CODE::min_quiet_usize) * (1+tolerance);
        }

        // scanns up to c bars (possibly less)
        // parameter: tokenizer, num of bars
        // accepts a parameter for maximal size of a bar (0 for unbounded)
        // returns num of bars that where read
        // modifies the paramter vector to hold result
        
        // FIXME make use of bound --> implement support in Tokenizer!
        uint add_bars(Tokenizer& tok, bar_vector_t& v, uint c, psize_t bound = 0)
        {
            size_t old_size = v.size();
            v.resize(old_size + c);
            for (uint i = 0; i < c; ++i) {
                if (tok.end()) {
                    v.resize(old_size + i);
                    return c;
                } else {
                    v[old_size + i] = tok.next();
                    v.psize += v[old_size+i].second;
                }
            }
            return c;
        }

        uint get_bars(Tokenizer& tok, bar_vector_t& v,uint c, psize_t bound = 0)
        {
            v.resize(c);
            v.psize = 0;
            for (uint i = 0; i < c; ++i) {
                if (tok.end()) {
                    v.resize(i);
                    return c;
                } else {
                    v[i] = tok.next();
                    v.psize += v[i].second;
                }
            }
            return c;
        }

        // u-value-handling:
        // * compute initial value
        // * use it until failure occurs
        // * get new value from failure token (psize/expected_modules)
        // * check that deviation is within tolerance
        // * use aggregate (e.g. mean. NOTE mean allows drifting...)

        // lookup returns code or failure

        uint modules_count(const bar_vector_t& v, u_t u)
        {
            uint result = 0;
            for (uint i = 0; i < v.size(); ++i) {
                result += lround(v[i].second/u);
            }
            return result;
        }

        // modulizer
        // translates a sequence of bars into an module_word (uint16_t)
        // for efficient lookup in array;
        // compute module_word from bar_vector, u-value
        // optional parameter: expected num of modules
        // retunrs module_word, num of modules, or failure (should be safe to use 0 -- or max_value?)

        // FIXME make use of m and return num of modules
        module_word_t get_module_word(const bar_vector_t& v, u_t u, usize_t m = 0)
        {
            //assert(modules_count(v,u) <= 16);
            usize_t msum = 0;
            module_word_t tmp = 0;
            for(uint i = 0 ; i < v.size(); ++i) {
                usize_t mc = lround(v[i].second/u); // FIXME check tolerance
                msum += mc;
                if ( ! (mc < 5 && mc > 0) ) return 0; // at most 2 bit
                tmp <<= mc;
                if (v[i].first) {
                    switch (mc) {
                    case 1: tmp |= 0x1; break;
                    case 2: tmp |= 0x3; break;
                    case 3: tmp |= 0x7; break;
                    case 4: tmp |= 0xF; break;
                    default: assert(false);
                    }
                }
            }
            if (msum != m) return 0;
            else {
                assert(modules_count(v,u) <= 16);
                return tmp;
            }
        }

        module_word_t get_module_word_adjust_u(const bar_vector_t& b, u_t& u, usize_t m)
        {
            module_word_t mw = get_module_word(b,u,m);
            if ( ! mw ) {
                // try to adjust u
                u_t new_u = b.psize / m;

                // if nothing changes it makes no sense to try again
                if (new_u == u) return 0;

                if ( fabs(new_u - u) <= u*0.4 ) {
                    u = (new_u*2 + u) / 3;
                } else {
                    return 0;
                }
                // and try again
                mw = get_module_word(b,u,m);
                return mw;
            }
            return mw;
        }


        bool get_parity(const module_word_t& w)
        {
            unsigned int tmp = 0;
            module_word_t x = w;
            while ( x != 0 ) { tmp += 1 & x; x >>= 1; }
            return 1 & tmp; // return parity bit
        }

        bool get_parity(const bar_vector_t& v, u_t u)
        {
            unsigned int tmp = 0;
            for (uint i = 0; i < v.size(); ++i) {
                if (v[i].first) tmp += lround(v[i].second/u); // FIXME check tolerance
            }
            return 1 & tmp; //  return parity bit
        }

    };

    // "" indicates no_entry
    std::string code128_t::decode128(code_set_t code_set, module_word_t mw) const
    {
        // TODO
        // assume first bit is 1 and last bit is 0
        // use only the 9 bits inbetween for lookup
        if ( ! ((1<<10)&mw) || (mw&1)) {
            return "";
        }
        mw &= ((1<<10)-1);
        mw >>= 1;

        int c = table[mw];
        if (c == no_entry) return "";
        if (c == 106) return std::string(1,END);
        switch (code_set) {
        case code_set_c:
            if (c < 100) {
                char str[2];
                sprintf(str,"%2d",(int)c);
                return std::string(str);
            } else {
                return std::string(1,caux[c-96]);
            }
            break;
        case code_set_b:
            if (c < 96) {
                return std::string(1,c+32);
            } else {
                return std::string(1,baux[c-96]);
            }
            break;
        case code_set_a:
            if (c < 64) {
                return std::string(1,c+32);
            } else if ( c < 96 ) {
                return std::string(1,c-64);
            } else {
                return std::string(1,aaux[c-96]);
            }
            break;
        default: assert(false);
        }
    }

    code128_t::code_set_t code128_t::shift_code_set(code_set_t code_set) const
    {
       switch (code_set) {
       case code_set_a: return code_set_b;
       case code_set_b: return code_set_a;
       default: return code_set;
       }
    }

    // FNC1 as first symbol indicates GS1-128
    //      in third or subsequent position: ascii 29.
    // TODO FNC2 (anywhere) concatenate barcode with next barcode
    // TODO FNC3 initialize or reprogram the barcode reader with the current code
    // TODO FNC4 switch to extended ascii (latin-1 as default)
    //      (quiet complicated usage refer to GS1 spec 5.3.3.4.2)
    scanner_result_t code128_t::scan(Tokenizer tokenizer,psize_t quiet_psize) const
    {
        using namespace scanner_utilities;

        // get x and y
        pos_t x = tokenizer.get_x();
        pos_t y = tokenizer.get_y();

        // try to match start marker
        bar_vector_t b(6);
        if ( get_bars(tokenizer,b,6) != 6) return scanner_result_t();

        // get a first guess for u
        u_t u = (double) b.psize / 11; // 11 is the number of modules of the start sequence

        // check if u is within max_u imposed by quiet_zone
        if ( u > max_u<code128_t>(quiet_psize) ) return scanner_result_t();

        // expect start sequence
        code_set_t cur_code_set;
        module_word_t mw = get_module_word(b,u,11);
        std::string result = decode128(code_set_a,mw);

        switch (result[0]) {
        case STARTA: cur_code_set = code_set_a; break;
        case STARTB: cur_code_set = code_set_b; break;
        case STARTC: cur_code_set = code_set_c; break;
        default: return scanner_result_t();
        }

        // initialize checksum:
        long checksum = table[mw]; 
        // we add it here already because in the first run pre_symbol will be multiplied by 0
        char pre_symbol = table[mw];

        std::string code = "";
        code_t type = code128;
        bool end = false;
        bool shift = false;
        ulong pos = 0;
        while (! end) { 
            // get symbol
            if ( get_bars(tokenizer,b,6) != 6 ) return scanner_result_t();
            module_word_t mw = get_module_word_adjust_u(b,u,11);
            if ( ! mw ) return scanner_result_t();
            result = decode128( (shift ? shift_code_set(cur_code_set) : cur_code_set), mw);
            shift = false;
            switch (result.size()) {
            case 0: return scanner_result_t();
            case 2: code += result; break;
            case 1: 
                    switch (result[0]) {
                    case END: end = true; break;
                    case SHIFT: shift = true; break;
                    case CODEA: cur_code_set = code_set_a; break;
                    case CODEB: cur_code_set = code_set_b; break;
                    case CODEC: cur_code_set = code_set_c; break;
                    case FNC1: 
                                if (pos == 1) {
                                    type = gs1_128;
                                } else {
                                  code.push_back(29);
                                }
                                break;
                    case FNC2:
                    case FNC3:
                    case FNC4: std::cerr << "WARNING: Function charaters for code128 are not supported for now." << std::endl;
                                break;
                    default: code += result;
                    }
                    break;
            default: return scanner_result_t();
            }

            // update checksum
            if ( ! end ) checksum += pos * pre_symbol;
            pre_symbol = table[mw];

            // increment pos
            ++pos;

#ifdef SCANNER_DEBUG
            std::cerr << "cur code: '" << code << "'" << std::endl;
#endif
        } 

        // expect a black bar of 2 modules to complete end
        if ( get_bars(tokenizer,b,1) != 1) return scanner_result_t();
        mw = get_module_word_adjust_u(b,u,2);
        if ( mw != 3) return scanner_result_t();

        // Checksum and return result
        if (  checksum % 103 != pre_symbol ) {
            std::cerr << "WARNING: checksum test for code128 failed on \""<< code << "\"." << std::endl;
            return scanner_result_t();
        } else {
            // remove checksum char from end of code
            code.resize(code.size()-1);
            return scanner_result_t(type,code,x,y);
        }
    }

    // TODO if sucessfull: Advance ModulizerIterator behind right quiet zone.
    //      (and skip this bar code in the future/ check for bottom quiet zone)

    // scanner_result_t() indicates failure
    scanner_result_t ean_t::scan(Tokenizer tokenizer, psize_t quiet_psize)
    {
        using namespace scanner_utilities;

        // get x and y
        pos_t x = tokenizer.get_x();
        pos_t y = tokenizer.get_y();

        // try to match start marker

        // try ean with 3 bars
        bar_vector_t b(3);
        if ( get_bars(tokenizer,b,3) != 3) return scanner_result_t();

        // get a first guess for u
        u_t u = (double) b.psize / 3.0; // 3 is the number of modules of the start sequence

        // check if u is within max_u imposed by quiet_zone
        if ( u > max_u<ean_t>(quiet_psize) ) return scanner_result_t();

        // expect start sequence
        module_word_t mw = get_module_word(b,u,3);
        char result = auxtable[mw];
        std::string code = "";
        if (result != normal_guard) return scanner_result_t();

#ifdef SCANNER_DEBUG
        std::cerr << "##### ean: (" << std::dec << x << ", " << y << "), unit=" 
            << u << ", quiet_psize =" << quiet_psize << " : " 
            << std::hex << mw << " --> " << (int) result << std::endl;
#endif

        // Ok, we found an ean start sequence, let's try to read the code:

        uint bps = 4; // (bars per symbol) a symbol has 4 bars
        module_word_t parities = 0;
        uint symbols_count = 0;
        result = 0;
        b.resize(bps);
        do {
            // get symbol
            if ( get_bars(tokenizer,b,4) != 4 ) return scanner_result_t();

            // decide if to go for a symbol (7 modules) or the center_guard (4 modules)
            // FIXME FIXME FIXME Getting this right is crucial! Do not use a value that is to big
            if ( fabs(((double)b.psize / 4.0) - u) <= (u * 0.2) ) break;
           
            // lets assume 7 modules
            module_word_t mw = get_module_word_adjust_u(b,u,7);
            if ( ! mw ) return scanner_result_t();
            result = table[mw];
            if (! result) return scanner_result_t();
            ++symbols_count;
            parities <<= 1;
            parities |= get_parity(mw);
            code += result;
#ifdef SCANNER_DEBUG
            std::cerr << "symbol: '" << result << "'" << std::endl;
#endif
        } while ( true ); // only way to leave is a failure return or the center_guard break above

        // check if we found a symbol at all
        if ( ! symbols_count ) return scanner_result_t();

        // consume the next bar (it belongs to the center_guard, that we expect)
        if (add_bars(tokenizer,b,1) != 1) return scanner_result_t();

        // expect the center guard (with 5 modules)
        mw = get_module_word_adjust_u(b,u,5);
        if ( ! mw  || auxtable[mw] != center_guard) return scanner_result_t();

        // TODO check for special guard (we need to implement add_bar() method

        // Decode the second half of the barcode // TODO
        for (uint i = 0; i < symbols_count; ++i) {

            if ( get_bars(tokenizer,b,4) != 4 ) return scanner_result_t();
            module_word_t mw = get_module_word_adjust_u(b,u,7);
            if ( ! mw ) return scanner_result_t();
            result = table[mw];
            if (! result) return scanner_result_t();
            code += result;

#ifdef SCANNER_DEBUG
                std::cerr << "symbol: '" << result << "'" << std::endl;
#endif
        }

        // expect normal guard
        if ( get_bars(tokenizer,b,3) != 3) return scanner_result_t();
        mw = get_module_word_adjust_u(b,u,3);
        if ( ! mw ) return scanner_result_t();
        result = auxtable[mw];
        if (result == normal_guard) {

            // TODO check right quiet zone

        } else if (result == add_on_guard) {

            // TODO
            assert(false && "TODO");

        } else {
            return scanner_result_t();
        }

        // get type and lookup bit 0 for ean13
        code_t type = ean;
        if (symbols_count == 6) {
            result = ean13table[parities];
            if (! result) return scanner_result_t();
            std::string tmp = "";
            tmp.push_back(result);
            code = tmp + code;
            type = (result == '0') ? upca : ean13;
        } else {
            type = ean8;
        }

        // adjust code_type
        // update code_type of modulizer (TODO)
        // Set parameter (most of all module_word_size)

        // scan modules according to code_type
        return scanner_result_t(type,code,x,y);

    }

};