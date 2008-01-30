/*
 * Copyright (c) 2008 Valentin Ziegler <valentin@exactcode.de>
 * Copyright (c) 2008 Susanne Klaus <susanne@exactcode.de>
 *
 */

#include <iostream>
#include <string>
#include <sstream>

#include "eps.hh"

bool EPSCodec::readImage (std::istream* stream, Image& image)
{
    return false;
}

bool EPSCodec::writeImage (std::ostream* stream, Image& image, int quality,
			   const std::string& compress)
{
	const int w = image.w;
	const int h = image.h;
	double dpi = image.xres ? image.xres : 72; // TODO: yres might be different
	double scale = 72 / dpi; //postscript native resolution is 72dpi.

	const char* decodeName = "Decode [0 1 0 1 0 1]";
	const char* deviceName = "DeviceRGB";
 	if (image.spp == 1) {
		deviceName = "DeviceGray";
		decodeName = "Decode [0 1]";
	}

	*stream << 
		"%!PS-Adobe-3.0 EPSF-3.0\n"
		"%%BoundingBox: 0 0 " << scale*(double)w << " " << scale*(double)h << "\n"
		"0 dict begin\n"
		"/" << deviceName << " setcolorspace\n"
		"<<\n"
		"   /ImageType 1\n"
		"   /Width " << w << " /Height " << h << "\n"
		"   /BitsPerComponent " << image.bps << "\n"
		"   /" << decodeName << "\n"
		"   /ImageMatrix [\n"
		"       " << 1.0 / scale << " 0.0\n"
		"       0.0 " << -1.0 / scale << "\n"
		"       0.0 " << h << "\n"
		"   ]\n"
		"   /DataSource currentfile /ASCIIHexDecode filter\n"
		">> image"
		<< std::endl;

	const int bytes = image.stride() * h;
	uint8_t* data = image.getRawData();
	for (int i = 0; i < bytes; ++i) {
		static const char nibble[] = "0123456789abcdef";
                stream->put(nibble[data[i] >> 4]);
                stream->put(nibble[data[i] & 0x0f]);
		
		if (i % 40 == 39 || i == bytes - 1)
			stream->put('\n');
	}

	*stream << "showpage\nend" << std::endl;

 	return true;
}

EPSCodec eps_loader;
