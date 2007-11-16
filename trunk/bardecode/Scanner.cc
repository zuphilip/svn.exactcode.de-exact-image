#include "Scanner.hh"

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
        case code39: return s << "code39";
        case code39_mod43: return s << "code39_mod43";
        case code39_ext: return s << "code39_ext";
        case code25i: return s << "code25i";
        default: return s << "unknown barcode type";
        }
    }

    struct code25i_t
    {
        code25i_t();

        static const int START_SEQUENCE = 0xA;
        static const int END_SEQUENCE = 0xD;
        static const char no_entry = 255;

        static const usize_t min_quiet_usize = 10;
        static const usize_t min_quiet_usize_right = 10;

        scanner_result_t scan(Tokenizer&, psize_t) const;
        bool check_bar_vector(const bar_vector_t& b,psize_t old_psize = 0) const;
        std::pair<module_word_t,module_word_t> get_keys(const bar_vector_t& b) const;

        DECLARE_TABLE(table,0x18);
    } code25i_impl;

    code25i_t::code25i_t()
    {
        INIT_TABLE(table,0x18,no_entry);
        PUT_IN_TABLE(table,0x6,'0');
        PUT_IN_TABLE(table,0x11,'1');
        PUT_IN_TABLE(table,0x9,'2');
        PUT_IN_TABLE(table,0x18,'3');
        PUT_IN_TABLE(table,0x5,'4');
        PUT_IN_TABLE(table,0x14,'5');
        PUT_IN_TABLE(table,0xC,'6');
        PUT_IN_TABLE(table,0x3,'7');
        PUT_IN_TABLE(table,0x12,'8');
        PUT_IN_TABLE(table,0xA,'9');
    }


    struct code39_t
    {
        code39_t();

        // NOTE: if the first char is SHIFT, then a further barcode is expected to
        // be concatenated.

        // Character set: A-Z0-9-. $/+% and DELIMITER

        // Extended set: full ascii (by use of shift characters)
        // Usage of extended set is not encoded, but needs to be requested explicitly.
        // The code is first scaned normaly. The result is translated afterwards
        // into extend encoding

        // Usage of checksum is not encoded, but needs to be requested explicitly.
        // If requested this is performed after completely having scanned the code.


        // Decoding is based on a binary encoding of the width of bars (rather than
        // modules). Since each symbol has 9 bars we need a table of size 512.
        // Otherwith we would have needed size 2^(13 modules - 2 constant modules) = 2048.
        // ((Maybe we could safe even a bit more by directly encoding 3 of 9 ???)

        scanner_result_t scan(Tokenizer&, psize_t) const;
        bool check_bar_vector(const bar_vector_t& b,psize_t old_psize = 0) const;
        module_word_t get_key(const bar_vector_t& b) const;
        bool expect_n(Tokenizer& tok, psize_t old_psize) const;

        static const char DELIMITER  = 254;
        static const char no_entry = 255;

        static const usize_t min_quiet_usize = 10;
        static const usize_t min_quiet_usize_right = 10;

        DECLARE_TABLE(table,512);
        DECLARE_TABLE(aux,128);
    } code39_impl;

    code39_t::code39_t()
    {
        INIT_TABLE(table,512,no_entry);
        PUT_IN_TABLE(table,0x34, '0');
        PUT_IN_TABLE(table,0x121, '1');
        PUT_IN_TABLE(table,0x61, '2');
        PUT_IN_TABLE(table,0x160, '3');
        PUT_IN_TABLE(table,0x31, '4');
        PUT_IN_TABLE(table,0x130, '5');
        PUT_IN_TABLE(table,0x70, '6');
        PUT_IN_TABLE(table,0x25, '7');
        PUT_IN_TABLE(table,0x124, '8');
        PUT_IN_TABLE(table,0x64, '9');
        PUT_IN_TABLE(table,0x109, 'A');
        PUT_IN_TABLE(table,0x49, 'B');
        PUT_IN_TABLE(table,0x148, 'C');
        PUT_IN_TABLE(table,0x19, 'D');
        PUT_IN_TABLE(table,0x118, 'E');
        PUT_IN_TABLE(table,0x58, 'F');
        PUT_IN_TABLE(table,0xD, 'G');
        PUT_IN_TABLE(table,0x10C, 'H');
        PUT_IN_TABLE(table,0x4C, 'I');
        PUT_IN_TABLE(table,0x1C, 'J');
        PUT_IN_TABLE(table,0x103, 'K');
        PUT_IN_TABLE(table,0x43, 'L');
        PUT_IN_TABLE(table,0x142, 'M');
        PUT_IN_TABLE(table,0x13, 'N');
        PUT_IN_TABLE(table,0x112, 'O');
        PUT_IN_TABLE(table,0x52, 'P');
        PUT_IN_TABLE(table,0x7, 'Q');
        PUT_IN_TABLE(table,0x106, 'R');
        PUT_IN_TABLE(table,0x46, 'S');
        PUT_IN_TABLE(table,0x16, 'T');
        PUT_IN_TABLE(table,0x181, 'U');
        PUT_IN_TABLE(table,0xC1, 'V');
        PUT_IN_TABLE(table,0x1C0, 'W');
        PUT_IN_TABLE(table,0x91, 'X');
        PUT_IN_TABLE(table,0x190, 'Y');
        PUT_IN_TABLE(table,0xD0, 'Z');
        PUT_IN_TABLE(table,0x85, '-');
        PUT_IN_TABLE(table,0x184, '.');
        PUT_IN_TABLE(table,0xC4, ' ');
        PUT_IN_TABLE(table,0xA8, '$');
        PUT_IN_TABLE(table,0xA2, '/');
        PUT_IN_TABLE(table,0x8A, '+');
        PUT_IN_TABLE(table,0x2A, '%');
        PUT_IN_TABLE(table,0x94, DELIMITER);

        INIT_TABLE(aux,128,no_entry);
        PUT_IN_TABLE(aux,(uint)'0', 0);
        PUT_IN_TABLE(aux,(uint)'1', 1);
        PUT_IN_TABLE(aux,(uint)'2', 2);
        PUT_IN_TABLE(aux,(uint)'3', 3);
        PUT_IN_TABLE(aux,(uint)'4', 4);
        PUT_IN_TABLE(aux,(uint)'5', 5);
        PUT_IN_TABLE(aux,(uint)'6', 6);
        PUT_IN_TABLE(aux,(uint)'7', 7);
        PUT_IN_TABLE(aux,(uint)'8', 8);
        PUT_IN_TABLE(aux,(uint)'9', 9);
        PUT_IN_TABLE(aux,(uint)'A', 10);
        PUT_IN_TABLE(aux,(uint)'B', 11);
        PUT_IN_TABLE(aux,(uint)'C', 12);
        PUT_IN_TABLE(aux,(uint)'D', 13);
        PUT_IN_TABLE(aux,(uint)'E', 14);
        PUT_IN_TABLE(aux,(uint)'F', 15);
        PUT_IN_TABLE(aux,(uint)'G', 16);
        PUT_IN_TABLE(aux,(uint)'H', 17);
        PUT_IN_TABLE(aux,(uint)'I', 18);
        PUT_IN_TABLE(aux,(uint)'J', 19);
        PUT_IN_TABLE(aux,(uint)'K', 20);
        PUT_IN_TABLE(aux,(uint)'L', 21);
        PUT_IN_TABLE(aux,(uint)'M', 22);
        PUT_IN_TABLE(aux,(uint)'N', 23);
        PUT_IN_TABLE(aux,(uint)'O', 24);
        PUT_IN_TABLE(aux,(uint)'P', 25);
        PUT_IN_TABLE(aux,(uint)'Q', 26);
        PUT_IN_TABLE(aux,(uint)'R', 27);
        PUT_IN_TABLE(aux,(uint)'S', 28);
        PUT_IN_TABLE(aux,(uint)'T', 29);
        PUT_IN_TABLE(aux,(uint)'U', 30);
        PUT_IN_TABLE(aux,(uint)'V', 31);
        PUT_IN_TABLE(aux,(uint)'W', 32);
        PUT_IN_TABLE(aux,(uint)'X', 33);
        PUT_IN_TABLE(aux,(uint)'Y', 34);
        PUT_IN_TABLE(aux,(uint)'Z', 35);
        PUT_IN_TABLE(aux,(uint)'-', 36);
        PUT_IN_TABLE(aux,(uint)'.', 37);
        PUT_IN_TABLE(aux,(uint)' ', 38);
        PUT_IN_TABLE(aux,(uint)'$', 39);
        PUT_IN_TABLE(aux,(uint)'/', 40);
        PUT_IN_TABLE(aux,(uint)'+', 41);
        PUT_IN_TABLE(aux,(uint)'%', 42);
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

        scanner_result_t scan(Tokenizer&, psize_t) const;
        std::string decode128(code_set_t code_set, module_word_t mw) const; 
        code_set_t shift_code_set(code_set_t code_set) const;
        module_word_t get_key(module_word_t mw) const;

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
        PUT_IN_TABLE(table,0x166,0);
        PUT_IN_TABLE(table,0x136,1);
        PUT_IN_TABLE(table,0x133,2);
        PUT_IN_TABLE(table,0x4C,3);
        PUT_IN_TABLE(table,0x46,4);
        PUT_IN_TABLE(table,0x26,5);
        PUT_IN_TABLE(table,0x64,6);
        PUT_IN_TABLE(table,0x62,7);
        PUT_IN_TABLE(table,0x32,8);
        PUT_IN_TABLE(table,0x124,9);
        PUT_IN_TABLE(table,0x122,10);
        PUT_IN_TABLE(table,0x112,11);
        PUT_IN_TABLE(table,0xCE,12);
        PUT_IN_TABLE(table,0x6E,13);
        PUT_IN_TABLE(table,0x67,14);
        PUT_IN_TABLE(table,0xE6,15);
        PUT_IN_TABLE(table,0x76,16);
        PUT_IN_TABLE(table,0x73,17);
        PUT_IN_TABLE(table,0x139,18);
        PUT_IN_TABLE(table,0x12E,19);
        PUT_IN_TABLE(table,0x127,20);
        PUT_IN_TABLE(table,0x172,21);
        PUT_IN_TABLE(table,0x13A,22);
        PUT_IN_TABLE(table,0x1B7,23);
        PUT_IN_TABLE(table,0x1A6,24);
        PUT_IN_TABLE(table,0x196,25);
        PUT_IN_TABLE(table,0x193,26);
        PUT_IN_TABLE(table,0x1B2,27);
        PUT_IN_TABLE(table,0x19A,28);
        PUT_IN_TABLE(table,0x199,29);
        PUT_IN_TABLE(table,0x16C,30);
        PUT_IN_TABLE(table,0x163,31);
        PUT_IN_TABLE(table,0x11B,32);
        PUT_IN_TABLE(table,0x8C,33);
        PUT_IN_TABLE(table,0x2C,34);
        PUT_IN_TABLE(table,0x23,35);
        PUT_IN_TABLE(table,0xC4,36);
        PUT_IN_TABLE(table,0x34,37);
        PUT_IN_TABLE(table,0x31,38);
        PUT_IN_TABLE(table,0x144,39);
        PUT_IN_TABLE(table,0x114,40);
        PUT_IN_TABLE(table,0x111,41);
        PUT_IN_TABLE(table,0xDC,42);
        PUT_IN_TABLE(table,0xC7,43);
        PUT_IN_TABLE(table,0x37,44);
        PUT_IN_TABLE(table,0xEC,45);
        PUT_IN_TABLE(table,0xE3,46);
        PUT_IN_TABLE(table,0x3B,47);
        PUT_IN_TABLE(table,0x1BB,48);
        PUT_IN_TABLE(table,0x147,49);
        PUT_IN_TABLE(table,0x117,50);
        PUT_IN_TABLE(table,0x174,51);
        PUT_IN_TABLE(table,0x171,52);
        PUT_IN_TABLE(table,0x177,53);
        PUT_IN_TABLE(table,0x1AC,54);
        PUT_IN_TABLE(table,0x1A3,55);
        PUT_IN_TABLE(table,0x18B,56);
        PUT_IN_TABLE(table,0x1B4,57);
        PUT_IN_TABLE(table,0x1B1,58);
        PUT_IN_TABLE(table,0x18D,59);
        PUT_IN_TABLE(table,0x1BD,60);
        PUT_IN_TABLE(table,0x121,61);
        PUT_IN_TABLE(table,0x1C5,62);
        PUT_IN_TABLE(table,0x98,63);
        PUT_IN_TABLE(table,0x86,64);
        PUT_IN_TABLE(table,0x58,65);
        PUT_IN_TABLE(table,0x43,66);
        PUT_IN_TABLE(table,0x16,67);
        PUT_IN_TABLE(table,0x13,68);
        PUT_IN_TABLE(table,0xC8,69);
        PUT_IN_TABLE(table,0xC2,70);
        PUT_IN_TABLE(table,0x68,71);
        PUT_IN_TABLE(table,0x61,72);
        PUT_IN_TABLE(table,0x1A,73);
        PUT_IN_TABLE(table,0x19,74);
        PUT_IN_TABLE(table,0x109,75);
        PUT_IN_TABLE(table,0x128,76);
        PUT_IN_TABLE(table,0x1DD,77);
        PUT_IN_TABLE(table,0x10A,78);
        PUT_IN_TABLE(table,0x3D,79);
        PUT_IN_TABLE(table,0x9E,80);
        PUT_IN_TABLE(table,0x5E,81);
        PUT_IN_TABLE(table,0x4F,82);
        PUT_IN_TABLE(table,0xF2,83);
        PUT_IN_TABLE(table,0x7A,84);
        PUT_IN_TABLE(table,0x79,85);
        PUT_IN_TABLE(table,0x1D2,86);
        PUT_IN_TABLE(table,0x1CA,87);
        PUT_IN_TABLE(table,0x1C9,88);
        PUT_IN_TABLE(table,0x16F,89);
        PUT_IN_TABLE(table,0x17B,90);
        PUT_IN_TABLE(table,0x1DB,91);
        PUT_IN_TABLE(table,0xBC,92);
        PUT_IN_TABLE(table,0x8F,93);
        PUT_IN_TABLE(table,0x2F,94);
        PUT_IN_TABLE(table,0xF4,95);
        PUT_IN_TABLE(table,0xF1,96);
        PUT_IN_TABLE(table,0x1D4,97);
        PUT_IN_TABLE(table,0x1D1,98);
        PUT_IN_TABLE(table,0xEF,99);
        PUT_IN_TABLE(table,0xF7,100);
        PUT_IN_TABLE(table,0x1AF,101);
        PUT_IN_TABLE(table,0x1D7,102);
        PUT_IN_TABLE(table,0x142,103);
        PUT_IN_TABLE(table,0x148,104);
        PUT_IN_TABLE(table,0x14e,105);
        PUT_IN_TABLE(table,0x11d,106);

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
        scanner_result_t scan(Tokenizer&, psize_t);

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

        while ( ! end() ) {

            token_t t = tokenizer.next();

            // goto next white space of psize >= min_quiet_psize
            while (t.first || t.second < min_quiet_psize) { // while black ...
                if ( end() ) return;
                else t = tokenizer.next();
            }
            assert(! t.first); // assert white

            /* ***************************** */
            // preliminary checks

            // FIXME move some of the computations into tokenizer by stopping the tokenizer
            // on linebreaks?
            
            // FIXME handle linebreaks in scanners: fail on linebreak;


            // consider line breaks; (alternatively we can stop the tokenizer at each linebreak!)
            psize_t quiet_psize = t.second < tokenizer.get_x() ? t.second : tokenizer.get_x();

            // each (non empty) Barcode has at least 24 modules (including both quiet zones)!
            if (tokenizer.get_x() + 17 > tokenizer.get_img()->w ) continue;

            // check quiet_zone against minimal requirement from all code types
            Tokenizer tmp_tok = tokenizer;
            if ( end() ) return;
            token_t lookahead = tmp_tok.next();
            if (lookahead.second * 3 > quiet_psize) {
                tokenizer = tmp_tok;
                continue;
            }

            // line break at black bar:
            if (tmp_tok.get_x() <= lookahead.second) continue;

            // not enough space left on line for minimal barcode width:
            if (lookahead.second/3.0 * 14 + tokenizer.get_x() >= tokenizer.get_img()->w) continue;

            /* ***************************** */

            Tokenizer backup_tokenizer = tokenizer;
            scanner_result_t result;
            // try scanning for all requested barcode types
            if (requested(code39)) {
                tokenizer =  backup_tokenizer;
                if ((result = code39_impl.scan(tokenizer,quiet_psize))) {
                    // TODO set tokenizer to end of barcode
                    cur_barcode = result;
                    return;
                }
            }
            if (requested(code25i)) {
                tokenizer =  backup_tokenizer;
                if ((result = code25i_impl.scan(tokenizer,quiet_psize))) {
                    // TODO set tokenizer to end of barcode
                    cur_barcode = result;
                    return;
                }
            }
            if (requested(code128)) {
                tokenizer =  backup_tokenizer;
                if (result = code128_impl.scan(tokenizer,quiet_psize)) {
                    // TODO set tokenizer to end of barcode
                    cur_barcode = result;
                    return;
                }
            } 
            if ( requested(ean) ) {
                tokenizer =  backup_tokenizer;
                if ((result = ean_impl.scan(tokenizer,quiet_psize)) ) {
                    // TODO set tokenizer to end of barcode
                    cur_barcode = result;
                    return;
                }
            } 
            tokenizer = backup_tokenizer;

            if ( end() ) return;
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
                    return i;
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
                    return i;
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

//#define STRICT // seems too strict!

    std::pair<module_word_t,module_word_t> code25i_t::get_keys(const bar_vector_t& b) const
    {
#ifdef STRICT
        u_t n_l = ((double) b.psize / 18.0); // (((b.size/2) / (3*1+2*3)) * 1
        u_t n_h = ((double) b.psize / 14.0); // (((b.size/2) / (3*1+2*2)) * 1
        u_t w_l = ((double) b.psize / 7.0);  // (((b.size/2) / (3*1+2*2)) * 2
        u_t w_h = ((double) b.psize / 6.0);   // (((b.size/2) / (3*1+2*3)) * 3
#else
        u_t n_l = ((double) b.psize / 36.0);
        u_t n_h = ((double) b.psize / 11.0);
        u_t w_l = ((double) b.psize / 10.0);
        u_t w_h = ((double) b.psize / 1.0);

#endif
        assert(b.size() == 10);
        module_word_t r1 = 0;
        module_word_t r2 = 0;
        for (uint i = 0; i < 10; ++i) {
            r1 <<= 1;
            if (w_l <= b[i].second && b[i].second <= w_h) r1 += 1;
            else if (! (n_l <= b[i].second && b[i].second <= n_h)) {
              //  std::cerr << "%%%%%%%% key1 failure: size=" << b[i].second << ", b.psize=" << b.psize << std::endl;
                return std::make_pair(0,0);
            }

            ++i;
            r2 <<= 1;
            if (w_l <= b[i].second && b[i].second <= w_h) r2 += 1;
            else if (! (n_l <= b[i].second && b[i].second <= n_h)) {
             //   std::cerr << "%%%%%%%% key2 failure: size=" << b[i].second << ", b.psize=" << b.psize << std::endl;
                return std::make_pair(0,0);
            }
        }
        return std::make_pair(r1,r2);
    }

    // psize = 0 means skip that test
    bool code25i_t::check_bar_vector(const bar_vector_t& b,psize_t old_psize) const
    {
        // check psize
        // check colors
        assert(b.size() == 10);
#if 0
        return 
            (!old_psize || fabs((long)b.psize - (long)old_psize) < 0.5 * old_psize) && 
            b[0].first && b[8].first;
#else
        if (old_psize && ! (fabs((long) b.psize - (long) old_psize) < 0.5 * old_psize)) {
            // std::cerr << "%%%%%%%% psize failure: old_psize=" << old_psize << ", new_psize=" << b.psize << std::endl;
            return false;
        }
        if ( ! b[0].first || b[9].first ) {
                // std::cerr << "%%%%%%%% color failure" << std::endl;
                return false;
        }
        return true;
#endif
    }


    scanner_result_t code25i_t::scan(Tokenizer& tokenizer, psize_t quiet_psize) const
    {
        using namespace scanner_utilities;

        // get x and y
        pos_t x = tokenizer.get_x();
        pos_t y = tokenizer.get_y();

        // try to match start marker
        // do relatively cheap though rough test on the first two bars only.
        bar_vector_t b(4);
        if ( get_bars(tokenizer,b,2) != 2 ) return scanner_result_t();
        if (b[0].second < 0.7 * b[1].second || b[0].second > 1.3 * b[1].second) return scanner_result_t();

        // check quiet_zone with respect to length of the first symbol
        if (quiet_psize < (double) b[0].second * 10 * 0.7) return scanner_result_t(); // 10 x quiet zone

        if ( add_bars(tokenizer,b,2) != 2 ) return scanner_result_t();
        if (b[0].second < 0.7 * b[2].second || b[0].second > 1.3 * b[2].second) return scanner_result_t();
        if (b[0].second < 0.7 * b[3].second || b[0].second > 1.3 * b[3].second) return scanner_result_t();

        u_t u = b.psize / 4.0;

        // std::cerr << "%%%%%%%% code25i: quiet zone ok" << std::endl;

        std::string code = "";
        psize_t old_psize = 0;
        bool end = false;
        while (! end) {

            // get new symbols
            if ( get_bars(tokenizer,b,3) != 3) return scanner_result_t();

            // check END sequence and expect quiet zone
            if ( (b.psize / (double) b[0].second) > 2 * 0.7 &&
                 (b.psize / (double) b[0].second) < 2 * 1.3 &&
                 (b.psize / (double) b[1].second) > 4 * 0.7 &&
                 (b.psize / (double) b[1].second) < 4 * 1.3 &&
                 (b.psize / (double) b[2].second) > 4 * 0.7 &&
                 (b.psize / (double) b[2].second) < 4 * 1.3) {
                // FIXME make this in a more performant way
                Tokenizer tok = tokenizer;
                token_t t = tok.next();
                if (t.second > b.psize * 2) {
                    break;
                }
            }

            if ( add_bars(tokenizer,b,7) != 7) return scanner_result_t();
            if (! check_bar_vector(b,old_psize) ) return scanner_result_t();
            old_psize = b.psize;

            // std::cerr << "%%%%%%%% code25i: got 7 bars" << std::endl;

            std::pair<module_word_t,module_word_t> keys = get_keys(b);
            if (! keys.first || ! keys.second ) return scanner_result_t();
            // std::cerr << "%%%%%%%% code25i - keys: " << std::hex << keys.first << " " << std::hex << keys.second << std::dec << std::endl;

            const char c1 = table[keys.first];
            // std::cerr << "%%%%%%%% code25i c1:" << c1 << std::endl;
            if (c1 == no_entry) return scanner_result_t();
            code.push_back(c1);

            const char c2 = table[keys.second];
            if (c2 == no_entry) return scanner_result_t();
            code.push_back(c2);
        }
        if ( code.empty() ) return scanner_result_t();
        else return scanner_result_t(code25i,code,x,y);
    }


    module_word_t code39_t::get_key(const bar_vector_t& b) const
    {
#ifdef STRICT
        u_t n_l = ((double) b.psize / 15.0); // ((b.size / (6*1+3*3)) * 1
        u_t n_h = ((double) b.psize / 12.0); // ((b.size / (6*1+3*2)) * 1
        u_t w_l = ((double) b.psize / 6.0);  // ((b.size / (6*1+3*2)) * 2
        u_t w_h = ((double) b.psize / 5.0);   // ((b.size / (6*1+3*3)) * 3
#else
        u_t n_l = ((double) b.psize / 25.0);
        u_t n_h = ((double) b.psize / 8.0);
        u_t w_l = ((double) b.psize / 7.9);
        u_t w_h = ((double) b.psize / 1.0);

#endif
        assert(b.size() == 9);
        module_word_t r = 0;
        for (uint i = 0; i < 9; ++i) {
            r <<= 1;
            if (w_l <= b[i].second && b[i].second <= w_h) r += 1;
            else if (! (n_l <= b[i].second && b[i].second <= n_h)) return 0;
        }
        return r;
    }

    // psize = 0 means skip that test
    bool code39_t::check_bar_vector(const bar_vector_t& b,psize_t old_psize) const
    {
        // check psize
        // check colors
        assert(b.size() == 9);
#if 0
        return 
            (!old_psize || fabs((long)b.psize - (long)old_psize) < 0.5 * old_psize) && 
            b[0].first && b[8].first;
#else
        if (old_psize && ! (fabs((long) b.psize - (long) old_psize) < 0.5 * old_psize)) {
    //        std::cerr << "%%%%%%%% psize failure: old_psize=" << old_psize << ", new_psize=" << b.psize << std::endl;
            return false;
        }
        if ( ! (b[0].first && b[8].first) ) {
    //            std::cerr << "%%%%%%%% color failure" << std::endl;
                return false;
        }
        return true;
#endif
    }

    bool code39_t::expect_n(Tokenizer& tok, psize_t old_psize) const
    {
        using namespace scanner_utilities;
        bar_vector_t b(1);
        if ( get_bars(tok,b,1) != 1 || b[0].first ) return false;
#ifdef STRICT
        u_t n_l = ((double) old_psize / 15.0); // ((b.size / (6*1+3*3)) * 1
        u_t n_h = ((double) old_psize / 12.0); // ((b.size / (6*1+3*2)) * 1
#else
        u_t n_l = ((double) old_psize / 25.0);
        u_t n_h = ((double) old_psize / 7.0);
#endif
        return n_l <= b[0].second && b[0].second <= n_h;
    }

    namespace debug
    {
        void print_bar_vector(const bar_vector_t& b)
        {
            std::cerr << "[ ";
            for (size_t i = 0; i < b.size(); ++i) {
                std::cerr << "(" << b[i].first << "," << b[i].second << ") ";
            }
            std::cerr << "]" << std::endl;
        }

    };
        

    scanner_result_t code39_t::scan(Tokenizer& tokenizer, psize_t quiet_psize) const
    {
        using namespace scanner_utilities;

        // get x and y
        pos_t x = tokenizer.get_x();
        pos_t y = tokenizer.get_y();

        //std::cerr << "%%%%%%%% try code39 at " << x << "," << y << std::endl;

        // try to match start marker
        // do relatively cheap though rough test on the first two bars only.
        bar_vector_t b(9);
        if ( get_bars(tokenizer,b,2) != 2 ) return scanner_result_t();
        if (b[0].second > 0.7 * b[1].second) return scanner_result_t();
        if (b[1].second > 3.3 * b[0].second) return scanner_result_t();

        if ( add_bars(tokenizer,b,7) != 7 ) return scanner_result_t();
        if (! check_bar_vector(b) ) return scanner_result_t();

        // check quiet_zone with respect to length of the first symbol
        if (quiet_psize < (double) b.psize * 0.7) return scanner_result_t(); // 10 x quiet zone

        // expect start sequence
        module_word_t key = get_key(b);
        if ( ! key || table[key] != DELIMITER ) {
            // std::cerr << "%%%%%%%% invalid DELIMITER failure: " << key << " --> " << table[key] << std::endl;
            return scanner_result_t();
        }

        std::string code = "";
        psize_t old_psize;
        bool end = false;
        while (! end) {
            old_psize = b.psize;
            // consume narrow separator
            if (! expect_n(tokenizer,old_psize)) return scanner_result_t();

            // get new symbol
            if ( get_bars(tokenizer,b,9) != 9) return scanner_result_t();
            if (! check_bar_vector(b,old_psize) ) return scanner_result_t();

            key = get_key(b);
            if (! key ) return scanner_result_t();
            const char c = table[key];
            switch(c) {
            case no_entry: return scanner_result_t();
            case DELIMITER: end = true; break;
            default: code.push_back(c);
            }
        }

        return scanner_result_t(code39,code,x,y);
    }


    // "" indicates no_entry
    std::string code128_t::decode128(code_set_t code_set, module_word_t key) const
    {
        int c = table[key];
        if (c == no_entry) return "";
        if (c == 106) return std::string(1,END);
        switch (code_set) {
        case code_set_c:
            if (c < 100) {
                char str[2];
                sprintf(str,"%02d",c);
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

    module_word_t code128_t::get_key(module_word_t mw) const
    {
        // assume first bit is 1 and last bit is 0
        // use only the 9 bits inbetween for lookup
        if ( ! ((1<<10)&mw) || (mw&1)) {
            return 0;
        }
        mw &= ((1<<10)-1);
        mw >>= 1;
        
        return mw;
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
    scanner_result_t code128_t::scan(Tokenizer& tokenizer,psize_t quiet_psize) const
    {
        using namespace scanner_utilities;

        // get x and y
        pos_t x = tokenizer.get_x();
        pos_t y = tokenizer.get_y();

        // try to match start marker
        bar_vector_t b(6);
        if (get_bars(tokenizer,b,2) != 2 ) return scanner_result_t();
        if (b[0].second > 3 * b[1].second || b[0].second < 1.2 * b[1].second) return scanner_result_t();
        if ( add_bars(tokenizer,b,4) != 4) return scanner_result_t();

        // get a first guess for u
        u_t u = (double) b.psize / 11; // 11 is the number of modules of the start sequence

        // check if u is within max_u imposed by quiet_zone
        if ( u > max_u<code128_t>(quiet_psize) ) return scanner_result_t();

        // expect start sequence
        code_set_t cur_code_set;
        module_word_t key = get_key(get_module_word(b,u,11));
        std::string result = decode128(code_set_a,key);

        switch (result[0]) {
        case STARTA: cur_code_set = code_set_a; break;
        case STARTB: cur_code_set = code_set_b; break;
        case STARTC: cur_code_set = code_set_c; break;
        default: return scanner_result_t();
        }

        // initialize checksum:
        long checksum = table[key]; 
        // we add it to checksum here already because in the first run pre_symbol will be multiplied by 0
        long pre_symbol = table[key];

        std::string code = "";
        code_t type = code128;
        bool end = false;
        bool shift = false;
        long pos = 0;
        while (! end) { 
            // get symbol
            if ( get_bars(tokenizer,b,6) != 6 ) return scanner_result_t();
            key = get_key(get_module_word_adjust_u(b,u,11));
            if ( ! key ) return scanner_result_t();
            result = decode128( (shift ? shift_code_set(cur_code_set) : cur_code_set), key);
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
            if ( ! end ) {
                checksum += pos * pre_symbol;
                pre_symbol = table[key];
            }

            // increment pos
            ++pos;

#ifdef SCANNER_DEBUG
            std::cerr << "cur code: '" << code << "'" << std::endl;
#endif
        } 

        // expect a black bar of 2 modules to complete end
        if ( get_bars(tokenizer,b,1) != 1) return scanner_result_t();
        module_word_t mw = get_module_word_adjust_u(b,u,2);
        if ( mw != 3) return scanner_result_t();

        // Checksum and return result
        if (  (checksum % 103) != pre_symbol ) {
            std::cerr << "WARNING: checksum test for code128 failed on \""<< code << "\"." << std::endl;
            std::cerr << "         checksum: " << checksum << " % 103 = " << checksum % 103 << " != " << pre_symbol << std::endl;
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
    scanner_result_t ean_t::scan(Tokenizer& tokenizer, psize_t quiet_psize)
    {
        using namespace scanner_utilities;

        // get x and y
        pos_t x = tokenizer.get_x();
        pos_t y = tokenizer.get_y();

        // try to match start marker

        // try ean with 3 bars
        bar_vector_t b(3);
        if ( get_bars(tokenizer,b,2) != 2) return scanner_result_t();
        if ( b[0].second > 2 * b[1].second || b[0].second < 0.5 * b[1].second ) return scanner_result_t();
        if ( add_bars(tokenizer,b,1) != 1) return scanner_result_t();

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
            if ( result == '0' ) {
                type = upca;
            } else {
                code = std::string(1,result) + code;
                type = ean13;
            }
        } else {
            type = ean8;
        }

        // checksum test
        int check = code[code.size()-1] - 48;
        code.erase(code.size()-1);
        int sum = 0;
        int f = 3;
        for (int i = (int)code.size()-1; i >= 0; --i) {
            sum += (code[i]-48)*f;
            f = ((f==1) ? 3 : 1);
        }
        if (10-(sum % 10) != check) return scanner_result_t();

        // scan modules according to code_type
        return scanner_result_t(type,code,x,y);

    }

};
