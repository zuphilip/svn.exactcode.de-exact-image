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

    // translates a sequence of bars into an module_word (uint16_t)
    // for efficient lookup in c array;
    module_word_t Scanner::get_module_word(const std::vector<bar_t>& v)
    {
        assert(v.size() <= 8); // at most bars of 4 values (2 bit)
        module_word_t tmp = 0;
        uint s = v.size();
        for(uint i = 0 ; i < s; ++i) {
            assert(v[i] < 4); // at most 2 bit
            tmp <<= 2;
            tmp |= v[i];
        }
        return tmp;
    }

    bool Scanner::get_parity(const module_word_t& w)
    {
        unsigned int tmp = 0;
        while ( w != 0 ) { tmp += 1 & w; w <<= 1; }
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


    DECLARE_TABLE(ean,128);
    DECLARE_TABLE(ean13_0,64);
    DECLARE_TABLE(eanaux,32);

    void init_tables() {

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
#define EANB(a) 0x99-((~a)&127) // mirror of EANC, i.e. 0x99 - EANC
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
};
