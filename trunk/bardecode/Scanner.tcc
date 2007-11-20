//#define SCANNER_DEBUG

#include "scanner_utils.hh"

#include "code128.hh"
#include "code25i.hh"
#include "code39.hh"
#include "ean.hh"

namespace BarDecode
{

    namespace
    {
        ean_t ean_impl;
        code128_t code128_impl;
        code39_t code39_impl;
        code25i_t code25i_impl;
    };

    // TODO make all tables static (would be nice to have something like designated initializers
    // like in C99 in C++ as well...) We do not have. Hecne we need to work around.
    // possibly by encapsulating the tables into objects with constructors that
    // perform the initialization.
    template<bool v>
    void BarcodeIterator<v>::next()
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
            psize_t quiet_psize = (pos_t) t.second < tokenizer.get_x() ? t.second : tokenizer.get_x();

            // each (non empty) Barcode has at least 24 modules (including both quiet zones)!
            if (tokenizer.get_x() + 17 > tokenizer.get_img()->w ) continue;

            // check quiet_zone against minimal requirement from all code types
            tokenizer_t tmp_tok = tokenizer;
            if ( end() ) return;
            token_t lookahead = tmp_tok.next();
            if (lookahead.second * 3 > quiet_psize) {
                tokenizer = tmp_tok;
                continue;
            }

            // line break at black bar:
            if (tmp_tok.get_x() <= (pos_t) lookahead.second) continue;

            // not enough space left on line for minimal barcode width:
            if (lookahead.second/3.0 * 14 + tokenizer.get_x() >= tokenizer.get_img()->w) continue;

            /* ***************************** */

            tokenizer_t backup_tokenizer = tokenizer;
            scanner_result_t result;
            // try scanning for all requested barcode types
            if (directions&left_right && requested(code39)) {
                tokenizer =  backup_tokenizer;
                if ((result = code39_impl.scan(tokenizer,quiet_psize))) {
                    cur_barcode = result;
                    return;
                }
            }
            if ( directions&right_left && requested(code39)) {
                tokenizer =  backup_tokenizer;
                if ((result = code39_impl.reverse_scan(tokenizer,quiet_psize))) {
                    cur_barcode = result;
                    return;
                }
            }
            if ( directions&left_right && requested(code25i)) {
                tokenizer =  backup_tokenizer;
                if ((result = code25i_impl.scan(tokenizer,quiet_psize))) {
                    cur_barcode = result;
                    return;
                }
            }
            if ( directions&right_left && requested(code25i)) {
                tokenizer =  backup_tokenizer;
                if ((result = code25i_impl.reverse_scan(tokenizer,quiet_psize))) {
                    cur_barcode = result;
                    return;
                }
            }
            if ( directions&left_right && requested(code128)) {
                tokenizer =  backup_tokenizer;
                if (result = code128_impl.scan(tokenizer,quiet_psize)) {
                    cur_barcode = result;
                    return;
                }
            } 
            if ( directions&right_left && requested(code128)) {
                tokenizer =  backup_tokenizer;
                if (result = code128_impl.reverse_scan(tokenizer,quiet_psize)) {
                    cur_barcode = result;
                    return;
                }
            } 
            if ( directions&(left_right|right_left) && requested(ean) ) {
                tokenizer =  backup_tokenizer;
                if ((result = ean_impl.scan(tokenizer,quiet_psize,directions)) ) {
                    cur_barcode = result;
                    return;
                }
            } 
            tokenizer = backup_tokenizer;

            if ( end() ) return;
        }
    }    
}; // namespace BarDecode
