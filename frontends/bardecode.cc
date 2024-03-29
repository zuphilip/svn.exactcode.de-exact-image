/*
 * The ExactImage library's any to multi-page TIFF converted
 * Copyright (C) 2007 - 2015 René Rebe, ExactCODE GmbH Germany
 * Copyright (C) 2007 Lars Kuhtz, ExactCODE GmbH Germany
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2. A copy of the GNU General
 * Public License can be found in the file LICENSE.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANT-
 * ABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details.
 *
 * Alternatively, commercial licensing options are available from the
 * copyright holder ExactCODE GmbH Germany.
 */

#include <math.h>

#include <iostream>
#include <iomanip>
#include <map>
#include <cctype>

#include "ArgumentList.hh"
#include "Codecs.hh"

#include "Tokenizer.hh"
#include "Scanner.hh"

#include "config.h"

using namespace Utility;

//#define BARDECODE_DEBUG
#ifdef BARDECODE_DEBUG
void down_test(Image& img)
{
    for (Image::const_iterator it = img.begin(); it != img.end(); it.down()) {
        *it;
    }
}
#endif


using namespace BarDecode;

namespace {

    struct comp {
        bool operator() (const scanner_result_t& a, const scanner_result_t& b) const
        {
            if (a.type < b.type) return true;
            else if (a.type > b.type) return false;
            else return (a.code < b.code);
        }
    };

    std::string filter_non_printable(const std::string& s)
    {
        std::string result;
        for (size_t i = 0; i < s.size(); ++i) {
            if ( std::isprint(s[i]) ) result.push_back(s[i]);
        }
        return result;
    }

};

int main (int argc, char* argv[])
{
  ArgumentList arglist (true); // enable residual gathering
  
  // setup the argument list
  Argument<bool> arg_help ("h", "help",
			   "display this help text and exit");
  Argument<int> arg_threshold ("t", "threshold",
			       "bi-level threshold value", 150, 0, 1);

  Argument<int> arg_concurrent_lines ("c", "concurrent-lines",
			       "number of lines that are scanned concurrently", 4, 0, 1);

  Argument<int> arg_line_skip ("s", "line-skip",
			       "number of lines that are skipped", 8, 0, 1);

  Argument<std::string> arg_format ("f", "format",
				    "user defined format string",
				    0, 1);

  Argument<int> arg_directions (
      "d", 
      "directions",
      "bitfield of directions to be scanned (0 none,1 left-to-right,2 top-down, 4 right-to-left, 8-down-top, 15 any)", 
      15, 0, 1);

  arglist.Add (&arg_help);
  arglist.Add (&arg_threshold);
  arglist.Add (&arg_directions);
  arglist.Add (&arg_concurrent_lines);
  arglist.Add (&arg_line_skip);
  arglist.Add (&arg_format);

  // parse the specified argument list - and maybe output the Usage
  if (!arglist.Read (argc, argv) || arg_help.Get() == true)
    {
      std::cerr << "ExactImage barcode recognition module, version " VERSION << std::endl
                << "Copyright (C) 2007 - 2015 René Rebe, ExactCODE GmbH" << std::endl
                << "Copyright (C) 2007 Lars Kuhtz, ExactCODE GmbH" << std::endl
                << "Usage:" << std::endl;
      
      arglist.Usage (std::cerr);
      return 1;
    }

  const std::vector<std::string>& filenames = arglist.Residuals();
  Image image;
  int errors = 0;
  
  std::string format;

  if (arg_format.Size() > 0) {
    format = arg_format.Get();
  } else {
    if (filenames.size() > 1)
      format = "%f: ";
    format += "%c [type: %t at: (%x,%y)]";
  }
  
  for (std::vector<std::string>::const_iterator file = filenames.begin();
       file != filenames.end ();
       ++file)
    {
      if (!ImageCodec::Read (*file, image)) {
	std::cerr << "Error reading " << *file << std::endl;
	++errors;
	continue;
      }
      
      int threshold = arg_threshold.Get();
      directions_t directions = (directions_t) arg_directions.Get();
      int concurrent_lines = arg_concurrent_lines.Get();
      int line_skip = arg_line_skip.Get();

      std::map<scanner_result_t,int,comp> codes;
      if (directions & (left_right | right_left)) {
          BarDecode::BarcodeIterator<> it(&image,threshold,ean|code128|gs1_128|code39|code25i,directions,concurrent_lines,line_skip);
          while (! it.end() ) {
              ++codes[*it];
              ++it;
          }
      }

      if (directions & (top_down | down_top)) {
          directions_t dir = (directions_t)((directions & (top_down | down_top)) >> 1);
          BarDecode::BarcodeIterator<true> it(&image,threshold,ean|code128|gs1_128|code39|code25i,dir,concurrent_lines,line_skip);
          while (! it.end() ) {
              ++codes[*it];
              ++it;
          }
      }
      
      for (std::map<scanner_result_t,int>::const_iterator it2 = codes.begin();
	   it2 != codes.end();
	   ++it2) {
	if (it2->first.type & (ean|code128|gs1_128) || it2->second > 1)
	  {
	    // output format sting with substitued escapes
	    for (std::string::const_iterator it = format.begin(); it != format.end(); ++it)
	    {
	      if (*it == '%') {
		if (++it == format.end())
		  --it; // print the % if at the end
		switch (*it) {
		case 'f': // filename
		  std::cout << *file; break;
		case 't': // type
		  std::cout << it2->first.type; break;
		case 'c': // content
		  std::cout << filter_non_printable(it2->first.code); break;
		case 'x': // x pos
		  std::cout << it2->first.x; break;
		case 'y': // y pos
		  std::cout << it2->first.y; break;
		case '%':
		  std::cout << *it; break;
		default:
		  if (it != format.begin())
		    --it;
		  std::cout << *it;
		}
	      }
	      else if (*it == '\\')
		{
		  if (++it == format.end())
		    --it; // print the \ if at the end
		  switch (*it) {
		  case 'n': std::cout << std::endl; break;
		  case 't': std::cout << "\t"; break;
		  case 'r': std::cout << "\r"; break;
		  case '\\': std::cout << *it; break;
		  default:
		    if (it != format.begin())
		      --it;
		    std::cout << *it;
		  }
		}
	      else
		std::cout << *it;
	    }
	    
	    std::cout << std::endl;
	  }
      }
#ifdef BARDECODE_DEBUG
      down_test(image);
#endif
      if (codes.empty())
	++errors;
    }
  return errors;
}
