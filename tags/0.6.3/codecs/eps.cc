/*
 * Copyright (c) 2008 Valentin Ziegler <valentin@exactcode.de>
 * Copyright (c) 2008 Susanne Klaus <susanne@exactcode.de>
 *
 */

#include "eps.hh"
#include "ps.hh"

bool EPSCodec::readImage (std::istream* stream, Image& image)
{
    return false;
}

bool EPSCodec::writeImage (std::ostream* stream, Image& image, int quality,
			   const std::string& compress)
{
	double dpi = image.xres ? image.xres : 72; // TODO: yres might be different
	double scale = 72 / dpi; //postscript native resolution is 72dpi.

	*stream << 
		"%!PS-Adobe-3.0 EPSF-3.0\n"
		"%%BoundingBox: 0 0 " << scale*(double)image.w << " " << scale*(double)image.h << "\n"
		"0 dict begin"
	<< std::endl;
	
	PSCodec::encodeImage (stream, image, scale, quality, compress);

	*stream << "showpage\nend" << std::endl;

 	return true;
}

EPSCodec eps_loader;
