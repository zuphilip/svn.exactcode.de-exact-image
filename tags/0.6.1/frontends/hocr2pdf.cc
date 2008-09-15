/*
 * The ExactImage library's hOCR to PDF
 * Copyright (C) 2008 René Rebe, ExactCODE GmbH Germany
 * Copyright (C) 2006 Archivista
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

#include <string.h>

#include <iostream>
#include <fstream>
#include <iomanip>
#include <cmath>
#include <cctype>

#include <vector>

#include "ArgumentList.hh"

#include "config.h"

#include "Codecs.hh"
#include "pdf.hh"

using namespace Utility;

int res = 300;
bool sloppy = false;
PDFCodec* pdfContext = 0;


std::string lowercaseStr(const std::string& _s)
{
  std::string s(_s);
  std::transform(s.begin(), s.end(), s.begin(), tolower);
  return s;
}

// custom copy to trip newlines, likewise
bool isMyBlank(char c)
{
  switch (c) {
  case ' ':
  case '\t':
  case '\r':
  case '\n':
    return true;
    break;
  default: return false;
  }
}

std::string peelWhitespaceStr(const std::string& _s)
{
  std::string s(_s);
  // trailing whitespace
  for (int i = s.size() - 1; i >= 0 && isMyBlank(s[i]); --i)
    s.erase(i, 1);
  // leading whitespace
  while (!s.empty() && isMyBlank(s[0]))
    s.erase(0, 1);
  return s;
}

// lower-case, and strip leading/trailing white-space
std::string sanitizeStr(const std::string& _s)
{
  return peelWhitespaceStr(lowercaseStr(_s));
}

// HTML decode

std::string htmlDecode(const std::string& _s)
{
  std::string s(_s);
  std::string::size_type i;
  
  while ((i = s.find("&amp;")) != std::string::npos)
    s.replace(i, 5, "&");

  while ((i = s.find("&lt;")) != std::string::npos)
    s.replace(i, 4, "<");
  
  while ((i = s.find("&gt;")) != std::string::npos)
    s.replace(i, 4, ">");
  
  // TODO: '&8212;' and more - when implemented, best locked on
  // each '&' and matched to the next ';'
  return s;
}


// state per char: bbox, bold, italic, boldItalic
// state per line: bbox, align: left, right justified

struct BBox {
  BBox()
    : x1(0), y1(0), x2(0), y2(0)
  {}
  
  bool operator== (const BBox& other)
  {
    return
      x1 == other.x1 &&
      y1 == other.y1 &&
      x2 == other.x2 &&
      y2 == other.y2;
  }
  
  double x1, y1, x2, y2;
} lastBBox;

std::ostream& operator<< (std::ostream& s, const BBox& b)
{
  s << b.x1 << ", " << b.y1 << ", " << b.x2 << ", " << b.y2;
  return s;
}

enum Style {
  None    = 0,
  Bold    = 1,
  Italic  = 2,
  BoldItalic = (Bold | Italic)
} lastStyle;

std::ostream& operator<< (std::ostream& s, const Style& st)
{
  switch (st) {
  case Bold: s << "Bold"; break;
  case Italic: s << "Italic"; break;
  case BoldItalic: s << "BoldItalic"; break;
  default: s << "None"; break;
  }
  return s;
}

// TODO: implement parsing, if of any guidance for the PDF
enum Align {
  Left    = 0,
  Right   = 1,
  Justify = 2,
} lastAlign;

struct Span {
  BBox bbox;
  Style style;
  std::string text;
};

struct Textline {
  std::vector<Span> spans;
  typedef std::vector<Span>::iterator span_iterator;
  
  void draw()
  {
    double y1 = 0, y2 = 0, yavg = 0;
    int n = 0;
    for (span_iterator it = spans.begin(); it != spans.end(); ++it, ++n)
      {
	if (it == spans.begin()) {
	  y1 = it->bbox.y1;
	  yavg = y2 = it->bbox.y2;
	} else {
	  if (it->bbox.y1 < y1)
	    y1 = it->bbox.y1;
	  if (it->bbox.y2 > y2)
	    y2 = it->bbox.y2;
	  yavg += it->bbox.y2;
	}
      }
    if (n > 0)
      yavg /= n;
    
    int height = (int)round(std::abs(y2 - y1) * 72. / res);
    if (height < 8)
      height = 8;
    
    //std::cerr << "drawing with height: " << height << std::endl;
    
    for (span_iterator it = spans.begin(); it != spans.end(); ++it, ++n)
      {
	// escape decoding, TODO: maybe change our SAX parser to emmit a single
	// text element, and thus decode it earlier
	std::string text = htmlDecode(it->text);
	BBox bbox = it->bbox;
	
	// one might imprecicely place text sloppily in favour of "sometimes"
	// improved cut'n paste-able text in not so advanced PDF Viewers
	if (sloppy) {
	  span_iterator it2 = it;
	  for (++it2; it2 != spans.end(); ++it2)
	    {
	      if (it->style != it2->style)
		break;
	      
	      std::string nextText = htmlDecode(it2->text);
	      
	      // TODO: in theory expand bbox, if later needed
	      text += nextText;
	      
	      // stop on whitespaces to sync on gaps in justified text
	      if (nextText != peelWhitespaceStr(nextText)) {
		++it2; // we consumed the glyph, so proceeed
		break;
	      }
	    }
	  it = --it2;
	}
	
	const char* font = "Helvetica";
	switch (it->style) {
	case Bold: 
	  font = "Helvetica-Bold"; break;
	case Italic:
	  font = "Helvetica-Oblique"; break;
	case BoldItalic:
	  font = "Helvetica-BoldOblique"; break;
	default:
	  ; // already initialized
	}
	
	//std::cerr << "(" << text << ") ";
	pdfContext->textTo(72. * bbox.x1 / res, 72. * yavg / res);
	pdfContext->showText(font, text, height);
      }
    //std::cerr << std::endl;
  }
  
  void flush()
  {
    if (!spans.empty())
      draw();
    spans.clear();
  }
  
  void push_back(Span s)
  {
    //std::cerr << "push_back (" << s.text << ") " << s.style << std::endl;
    
    // do not insert newline garbage (empty string after white-
    // space peeling) at the beginning of a line
    if (spans.empty()) {
      s.text = peelWhitespaceStr(s.text);
      if (s.text.empty())
	return;
    }
    
    // if the direction wrapps, assume new line
    if (!spans.empty() && s.bbox.x1 < spans.back().bbox.x1)
      flush();
    
    // unify inserted spans with same properties
    if (!spans.empty() &&
	(spans.back().bbox == s.bbox) &&
	(spans.back().style == s.style))
      spans.back().text += s.text;
    else
      spans.push_back(s);
  }
  
} textline;


BBox parseBBox(std::string s)
{
  BBox b; // self initialized to zero
  
  const char* tS = "title=\"";
  
  std::string::size_type i = s.find(tS);
  if (i == std::string::npos)
    return b;
  
  std::string::size_type i2 = s.find("\"", i + strlen(tS));
  if (i2 == std::string::npos)
    return b;
  
  std::stringstream stream(s.substr(i + strlen(tS), i2 - i - strlen(tS)));
  std::string dummy;
  stream >> dummy >> b.x1 >> b.y1 >> b.x2 >> b.y2;

  return b;
}

void elementStart(const std::string& _name, const std::string& _attr = "")
{
  std::string name(sanitizeStr(_name)), attr(sanitizeStr(_attr));
  
  //std::cerr << "elementStart: '" << name << "', attr: '" << attr << "'" << std::endl;
  
  BBox b = parseBBox(attr);
  if (b.x2 > 0 && b.y2 > 0)
    lastBBox = b;
  
  if (name == "b" || name == "strong")
    lastStyle = Style(lastStyle | Bold);
  else if (name == "i" || name == "em")
    lastStyle = Style(lastStyle | Italic);
  
}

void elementText(const std::string& text)
{
  Span s;
  s.bbox = lastBBox;
  s.style = lastStyle;
  s.text += text;
  
  textline.push_back(s);
}

void elementEnd(const std::string& _name)
{
  std::string name (sanitizeStr(_name));
  
  //std::cerr << "elementEnd: " << name << std::endl;
  
  if (name == "b" || name == "strong")
    lastStyle = Style(lastStyle & ~Bold);
  else if (name == "i" || name == "em")
    lastStyle = Style(lastStyle & ~Italic);
  
  // explicitly flush line of text on manual preak or end of paragraph
  else if (name == "br" || name == "p")
    textline.flush();
}


// returns the string before the first whitespace
std::string tagName(std::string t)
{
  std::string::size_type i = t.find(' ');
  if (i != std::string::npos)
    t.erase(i);
  return t;
}

int main(int argc, char* argv[])
{
  ArgumentList arglist(false);
  
  // setup the argument list
  Argument<bool> arg_help("h", "help",
			  "display this help text and exit");
  arglist.Add(&arg_help);
  
  Argument<std::string> arg_input("i", "input",
				  "input image filename",
				  1, 1, true, true);
  arglist.Add(&arg_input);

  Argument<std::string> arg_output("o", "output",
				   "output PDF filename",
				   1, 1, true, true);
  arglist.Add(&arg_output);
  
  Argument<int> arg_resolution("r", "resolution",
			       "resolution overwrite",
			       0, 1, true, true);
  arglist.Add(&arg_resolution);
  
  Argument<bool> arg_no_image("n", "no-image",
			      "do not place the image over the text",
			      0, 0, true, true);
  arglist.Add(&arg_no_image);

  Argument<bool> arg_sloppy_text("s", "sloppy-text",
				 "sloppily place text, group words, do not draw single glyphs",
				 0, 0, true, true);
  arglist.Add(&arg_sloppy_text);
  
  // parse the specified argument list - and maybe output the Usage
  if (!arglist.Read(argc, argv) || arg_help.Get() == true)
    {
      std::cerr << "hOCR to PDF converter, version " VERSION << std::endl
		<< "Copyright (C) 2008 René Rebe, ExactCODE" << std::endl
		<< "Copyright (C) 2008 Archivista" << std::endl
		<< "Usage:" << std::endl;
      
      arglist.Usage(std::cerr);
      return 1;
    }

  // load the image, if specified and possible
  
  Image image; image.w = image.h = 0;
  if (arg_input.Size())
    {
      if (!ImageCodec::Read(arg_input.Get(), image)) {
	std::cerr << "Error reading input file." << std::endl;
	return 1;
      }
    }

  if (arg_resolution.Size())
    image.xres = image.yres = arg_resolution.Get();
  if (image.xres <= 0 || image.yres <= 0) {
    std::cerr << "Warning: Image x/y resolution not set, defaulting to: " << res << std::endl;
    image.xres = image.yres = res; // default 300
  }
  res = image.xres;
  sloppy = arg_sloppy_text.Get();
  
  std::ofstream s(arg_output.Get().c_str());
  pdfContext = new PDFCodec(&s);
  pdfContext->beginPage(72. * image.w / res, 72. * image.h / res);
  pdfContext->setFillColor(0, 0, 0);
  
  // TODO: soft hyphens
  // TODO: better text placement, using one TJ with spacings
  // TODO: more iamge compressions, jbig2, Fax
  
  pdfContext->beginText();
  
  // minimal, cuneiform HTML ouptut parser
  char c;
  std::vector<std::string> openTags;  
  std::string* curTag = 0;
  std::string closingTag;
  while (std::cin.get(c), !std::cin.eof()) {
    // consume tag element text
    if (curTag && c != '>') {
      *curTag += c;
      continue;
    }
    
    switch (c) {
    case '<':
      if (std::cin.peek() != '/') {
	openTags.push_back("");
	curTag = &openTags.back();
      } else {
	closingTag.clear();
	curTag = &closingTag;
      }
      break;
    case '>':
      if (curTag != &closingTag) {
	bool closed = false;
	if (!curTag->empty() && curTag->at(curTag->size() - 1) == '/')
	  {
	    curTag->erase(curTag->size() - 1);
	    closed = true;
	  }
	
	// HTML asymetric tags, TODO: more of those?
	{
	  std::string lowTag = lowercaseStr(tagName(*curTag));
	  if (lowTag == "br" || lowTag == "img")
	    closed = true;
	}
	
	//std::cout << "tag start: " << openTags.back()
	//          << (closed ? " immediately closed" : "") << std::endl;
	{
	  std::string element = tagName(*curTag);
	  std::string attr = *curTag;
	  attr.erase(0, element.size());
	  elementStart(element, attr);
	}
	
	if (closed) {
	  elementEnd(*curTag);
	  openTags.pop_back();
	}
      }
      else {
	// garuanteed to begin with a /, remove it
	curTag->erase(0, 1);
	// get just the tag name from the stack
	std::string lastOpenTag = (openTags.empty() ? "" : openTags.back());
	lastOpenTag = tagName(lastOpenTag);
	if (lastOpenTag != *curTag) {
	  std::cout << "Warning: tag mismatch: '" << *curTag
		    << "' can not close last open: '"
		    << lastOpenTag
		    << "'" << std::endl;
	}
	else
	  openTags.pop_back();
	elementEnd(*curTag);
      }
      curTag = 0;
      break;
      
    default:
      elementText(std::string(1, c));
      break;
    }
  }
  
  while (!openTags.empty()) {
    std::string tag = tagName(openTags.back()); openTags.pop_back();
    // skip special tags such as !DOCTYPE
    if (tag.empty() || tag[0] != '!')
      std::cerr << "Warning: unclosed tag: '" << tag << "'" << std::endl;
  }
  
  textline.flush();
  
  pdfContext->endText();
  
  if (!arg_no_image.Get())
    pdfContext->showImage(image, 0, 0, 72. * image.w / res, 72. * image.h / res);
  
  delete pdfContext;
  return 0;
}
