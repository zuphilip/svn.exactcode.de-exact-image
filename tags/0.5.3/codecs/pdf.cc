/*
 *
 * Copyright (c) 2008 Susanne Klaus <susanne@exactcode.de>
 *
 */

#include <string>

#include "pdf.hh"
#include "Encodings.hh"


#if WITHLIBJPEG == 1
#include "jpeg.hh"
#endif

#if WITHJASPER == 1
#include "jpeg2000.hh"
#endif

bool PDFCodec::readImage (std::istream* stream, Image& image)
{
    return false;
}

bool PDFCodec::writeImage (std::ostream* stream, Image& image, int quality,
			   const std::string& compress)
{
	const int w = image.w;
	const int h = image.h;

	std::string encoding = "ASCII85Decode";

	if (!compress.empty())
	{
		std::string c (compress);
		std::transform (c.begin(), c.end(), c.begin(), tolower);

		if (c == "encodeascii85")
			encoding = "ASCII85Decode";
		else if (c == "encodehex")
			encoding = "ASCIIHexDecode";
		else if (c == "encodejpeg")
			encoding = "DCTDecode";
		else if (c == "encodejpeg2000")
			encoding = "JPXDecode";
		else
			std::cerr << "PDFCodec: Unrecognized encoding option '" << compress << "'" << std::endl;
	}

	const char* deviceName = "DeviceRGB";
	const char* imageColor = "ImageC";

 	if (image.spp == 1) {
		deviceName = "DeviceGray";
		imageColor = "ImageB";
	}

	std::vector <long> objs_offset;

	*stream << "%PDF-1.4\n";
	objs_offset.push_back(stream->tellp());
	*stream << "" << objs_offset.size() << " 0 obj\n"
		"<<\n"
		"	/Type /Catalog\n"
		"	/Pages 2 0 R\n"
		">>\n"
		"endobj\n";

	objs_offset.push_back(stream->tellp());
	*stream << "" << objs_offset.size() << " 0 obj\n"
		"<<\n"
		"	/Type /Pages\n"
		"	/Kids [3 0 R]\n"
		"	/Count 1\n"
		">>\n"
		"endobj\n";

	objs_offset.push_back(stream->tellp());
	*stream << "" << objs_offset.size() << " 0 obj\n"
		"<<\n"
		"	/Type /Page\n"
		"	/Parent 2 0 R\n"
		"	/Resources " << objs_offset.size() + 1 << " 0 R\n"
		"	/MediaBox [0 0 " << w << " " << h << "]\n"
		"	/Contents " << objs_offset.size() + 4 << " 0 R\n"
		">>\n"
		"endobj\n";

	// Resources
	objs_offset.push_back(stream->tellp());
	*stream << "" << objs_offset.size() << " 0 obj\n"
		"<<\n"
		"	/ProcSet [/PDF /" << imageColor << "]\n"
		"	/XObject << /Im1 " << objs_offset.size() + 1 << " 0 R >>\n"
		">>\n"
		"endobj\n";

	objs_offset.push_back(stream->tellp());
	*stream << "" << objs_offset.size() << " 0 obj\n"
		"<<\n"
		"	/Type /XObject\n"
		"	/Subtype /Image\n"
		"	/Width " << w << " /Height " << h << "\n"
		"	/ColorSpace /" << deviceName << "\n"
		"	/BitsPerComponent " << image.bps << "\n"
		"	/Length " << objs_offset.size() + 1 << " 0 R\n"
		"	/Filter /" << encoding << "\n"
		">>\n"
		"stream\n";


	long beginData = stream->tellp();
	const int bytes = image.stride() * h;
	uint8_t* data = image.getRawData();
	if (encoding == "ASCII85Decode")
		EncodeASCII85(*stream, data, bytes);
	else if (encoding == "ASCIIHexDecode")
		EncodeHex(*stream, data, bytes);
#if WITHLIBJPEG == 1
	else if (encoding == "DCTDecode") {
		JPEGCodec codec;
		codec.writeImage (stream, image, quality, compress);
	}
#endif
#if WITHJASPER == 1
	else if (encoding == "JPXDecode") {
		JPEG2000Codec codec;
		codec.writeImage (stream, image, quality, compress);
	}
#endif
	long endData = stream->tellp();

	*stream << "\nendstream\n"
		"endobj\n";

	objs_offset.push_back(stream->tellp());
	*stream << "" << objs_offset.size() << " 0 obj\n"
		"" << endData - beginData << "\n"
		"endobj\n";

	// Contents
	objs_offset.push_back(stream->tellp());
	*stream << "" << objs_offset.size() << " 0 obj\n"
		"<<\n"
		"	/Length " << objs_offset.size() + 1 << " 0 R\n"
		">>\n"
		"stream\n";

	beginData = stream->tellp();
	*stream << "q\n" << w << " 0 0 " << h << " 0 0 cm\n/Im1 Do\nQ\n";
	endData = stream->tellp();

	*stream << "endstream\n"
		"endobj\n";

	objs_offset.push_back(stream->tellp());
	*stream << "" << objs_offset.size() << " 0 obj\n"
		"" << endData - beginData << "\n"
		"endobj\n";

	long last_cross_reference = stream->tellp();
	*stream << "xref\n" // last cross-reference
		"0 " << objs_offset.size()+1 << "\n"
		"0000000000 65535 f\r\n";

	for (unsigned int i = 0; i < objs_offset.size(); ++i) {
		stream->fill('0');
		stream->width(10);
		*stream << std::right << objs_offset[i] << " 00000 n\r\n";
	}

	*stream << "\ntrailer\n"
		"<<\n"
		"	/Size " << objs_offset.size() + 1 << "\n" // total number of entries
		"	/Root 1 0 R\n" // indirect reference to catalog dictonary
		">>\n"
		"\nstartxref\n"
		"" << last_cross_reference << "\n"
		"%%EOF\n";

 	return true;
}

PDFCodec pdf_loader;
