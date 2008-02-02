/*
 *
 * Copyright (c) 2008 Susanne Klaus <susanne@exactcode.de>
 *
 */

#include <string>

#include "pdf.hh"

bool PDFCodec::readImage (std::istream* stream, Image& image)
{
    return false;
}

bool PDFCodec::writeImage (std::ostream* stream, Image& image, int quality,
			   const std::string& compress)
{
	const int w = image.w;
	const int h = image.h;

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
		"	/Filter /ASCIIHexDecode\n"
		">>\n"
		"stream\n";

	std::string imageData;
	const int bytes = image.stride() * h;
	uint8_t* data = image.getRawData();
	for (int i = 0; i < bytes; ++i) {
		static const char nibble[] = "0123456789abcdef";
		imageData += nibble[data[i] >> 4];
		imageData += nibble[data[i] & 0x0f];

		if (i % 40 == 39 || i == bytes - 1)
			imageData += '\n';
	}
	*stream << imageData;

	*stream << "endstream\n"
		"endobj\n";

	objs_offset.push_back(stream->tellp());
	*stream << "" << objs_offset.size() << " 0 obj\n"
		"" << imageData.length() << "\n"
		"endobj\n";

	// Contents
	objs_offset.push_back(stream->tellp());
	*stream << "" << objs_offset.size() << " 0 obj\n"
		"<<\n"
		"	/Length " << objs_offset.size() + 1 << " 0 R\n"
		">>\n"
		"stream\n";

	std::string imageData2;
	imageData2 += "q\n512 0 0 512 0 0 cm\n/Im1 Do\nQ\n";
	*stream << imageData2;

	*stream << "endstream\n"
		"endobj\n";

	objs_offset.push_back(stream->tellp());
	*stream << "" << objs_offset.size() << " 0 obj\n"
		"" << imageData2.length() << "\n"
		"endobj\n";

	long last_cross_reference = stream->tellp();
	*stream << "xref\n" // last cross-reference
		"0 " << objs_offset.size()+1 << "\n"
		"0000000000 65535 f\r\n";

	for (int i = 0; i < objs_offset.size(); ++i) {
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
