/*
 * Copyright (c) 2008 Valentin Ziegler <valentin@exactcode.de>
 * Copyright (c) 2008 Susanne Klaus <susanne@exactcode.de>
 *
 */

#include "ps.hh"
#include "Encodings.hh"

bool PSCodec::readImage (std::istream* stream, Image& image)
{
    return false;
}

bool PSCodec::writeImage (std::ostream* stream, Image& image, int quality,
			   const std::string& compress)
{
	const int w = image.w;
	const int h = image.h;
	double dpi = image.xres ? image.xres : 72; // TODO: yres might be different
	double scale = 72 / dpi; //postscript native resolution is 72dpi.

	const char* creatorName = "ExactImage";

	*stream <<
		"%!PS-Adobe-3.0\n"
		"%%Creator:" << creatorName << "\n"
		"%%DocumentData: Clean7Bit\n"
		"%%LanguageLevel: 2\n"
		"%%BoundingBox: 0 0 " << scale*(double)w << " " << scale*(double)h << "\n"
		"%%EndComments\n"
		"%%BeginProlog\n"
		"0 dict begin\n"
		"%%EndProlog\n"
		"%%BeginPage\n"
	<< std::endl;	

	encodeImage (stream, image, scale);

	*stream << "%%EndPage\nshowpage\nend" << std::endl;

 	return true;
}

void PSCodec::encodeImage (std::ostream* stream, Image& image, double scale)
{
	const int w = image.w;
	const int h = image.h;

	const char* decodeName = "Decode [0 1 0 1 0 1]";
	const char* deviceName = "DeviceRGB";
	const char* encoding = "ASCII85Decode";

 	if (image.spp == 1) {
		deviceName = "DeviceGray";
		decodeName = "Decode [0 1]";
	}

	if (false /* TODO: evaluate some command line option here */)
	  encoding = "ASCIIHexDecode";

	*stream << 
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
	  "   /DataSource currentfile /" << encoding << " filter\n"
		">> image"
		<< std::endl;

	const int bytes = image.stride() * h;
	uint8_t* data = image.getRawData();
	EncodeASCII85(*stream, data, bytes);
	//EncodeHex(*stream, data, bytes);
	stream->put('\n');
}

PSCodec ps_loader;
