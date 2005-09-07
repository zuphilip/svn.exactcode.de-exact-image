

unsigned char*
read_TIFF_file (const char* file, int* w, int* h, int* bps, int* spp,
		int* xres, int* yres);

void
write_TIFF_file (const char* file, unsigned char* data, int w, int h, int bps, int spp,
		 int xres, int yres);
