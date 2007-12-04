#ifndef _SCANNER_UTILS_HH_
#define _SCANNER_UTILS_HH_

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
    namespace {
        
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
        template<class TIT>
        uint add_bars(TIT& start, TIT end, bar_vector_t& v, uint c, psize_t bound = 0)
        {
            size_t old_size = v.size();
            v.resize(old_size + c);
            for (uint i = 0; i < c; ++i) {
                if (start == end) {
                    v.resize(old_size + i);
                    return i;
                } else {
                    v[old_size + i] = *++start;
                    v.psize += v[old_size+i].second;
                }
            }
            return c;
        }

        template<class TIT>
        uint get_bars(TIT& start, TIT end, bar_vector_t& v,uint c, psize_t bound = 0)
        {
            v.resize(c);
            v.psize = 0;
            for (uint i = 0; i < c; ++i) {
                if (start == end) {
                    v.resize(i);
                    return i;
                } else {
                    v[i] = *++start;
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

        module_word_t reverse_get_module_word(const bar_vector_t& v, u_t u, usize_t m = 0)
        {
            //assert(modules_count(v,u) <= 16);
            usize_t msum = 0;
            module_word_t tmp = 0;
            for(int i = (int)v.size()-1 ; i >= 0; --i) {
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
                u_t new_u = b.psize / (double) m;

                // if nothing changes it makes no sense to try again
                if (new_u == u) return 0;

                if ( fabs(new_u - u) <= u*0.4 ) {
                    u = (new_u*2.0 + u) / 3.0;
                } else {
                    return 0;
                }
                // and try again
                mw = get_module_word(b,u,m);
                if ( ! mw ) {
                    mw = get_module_word(b,u*0.75,m);
                    if (! mw ) mw = get_module_word(b,u*1.25,m);
                }
                return mw;
            }
            return mw;
        }

        module_word_t reverse_get_module_word_adjust_u(const bar_vector_t& b, u_t& u, usize_t m)
        {
            module_word_t mw = reverse_get_module_word(b,u,m);
            if ( ! mw ) {
                // try to adjust u
                u_t new_u = b.psize / (double) m;

                // if nothing changes it makes no sense to try again
                if (new_u == u) return 0;

                if ( fabs(new_u - u) <= u*0.4 ) {
                    u = (new_u*2.0 + u) / 3.0;
                } else {
                    return 0;
                }
                // and try again
                mw = reverse_get_module_word(b,u,m);
                if ( ! mw ) {
                    mw = reverse_get_module_word(b,u*0.75,m);
                    if (! mw ) mw = reverse_get_module_word(b,u*1.25,m);
                }
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
    };

}; // namespace BarDecode

#endif // _SCANNER_UTILS_HH_
